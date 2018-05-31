#include <graphene/chain/wasm_interface.hpp>
#include <graphene/chain/exceptions.hpp>
#include <boost/core/ignore_unused.hpp>
#include <graphene/chain/wasm_interface_private.hpp>
#include <graphene/chain/wasm_gxb_validation.hpp>
#include <graphene/chain/wasm_gxb_injection.hpp>

#include <fc/exception/exception.hpp>
#include <fc/crypto/sha256.hpp>
#include <fc/crypto/sha1.hpp>
#include <fc/crypto/hex.hpp>
#include <fc/io/raw.hpp>

#include <softfloat.hpp>
#include <compiler_builtins.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <fstream>

namespace graphene { namespace chain {
   using namespace webassembly;
   using namespace webassembly::common;

   wasm_interface::wasm_interface(vm_type vm) : my( new wasm_interface_impl(vm) ) {}

   wasm_interface::~wasm_interface() {}

   void wasm_interface::validate(const bytes& code) {
      Module module;
      try {
         Serialization::MemoryInputStream stream((U8*)code.data(), code.size());
         WASM::serialize(stream, module);
      } catch(const Serialization::FatalSerializationException& e) {
         FC_ASSERT(false, wasm_serialization_error, e.message.c_str());
      } catch(const IR::ValidationException& e) {
         FC_ASSERT(false, wasm_serialization_error, e.message.c_str());
      }

      wasm_validations::wasm_binary_validation validator(module);
      validator.validate();

      root_resolver resolver(true);
      LinkResult link_result = linkModule(module, resolver);

      //there are a couple opportunties for improvement here--
      //Easy: Cache the Module created here so it can be reused for instantiaion
      //Hard: Kick off instantiation in a separate thread at this location
	   }

   void wasm_interface::apply( const digest_type& code_id, const bytes& code, apply_context& context ) {
      my->get_instantiated_module(code_id, code)->apply(context);
   }

   wasm_instantiated_module_interface::~wasm_instantiated_module_interface() {}
   wasm_runtime_interface::~wasm_runtime_interface() {}

#if defined(assert)
   #undef assert
#endif

class context_aware_api {
   public:
      context_aware_api(apply_context& ctx, bool context_free = false )
      :context(ctx)
      {
      }

   protected:
      apply_context&             context;

};

class call_depth_api : public context_aware_api {
   public:
      call_depth_api( apply_context& ctx )
      :context_aware_api(ctx,true){}
      void call_depth_assert() {
         FC_THROW_EXCEPTION(wasm_execution_error, "Exceeded call depth maximum");
      }
};

class console_api : public context_aware_api {
   public:
      console_api( apply_context& ctx )
      : context_aware_api(ctx,true)
      , ignore(true) {}

      // Kept as intrinsic rather than implementing on WASM side (using prints_l and strlen) because strlen is faster on native side.
      void prints(null_terminated_ptr str) {
         if ( !ignore ) {
            context.console_append<const char*>(str);
         }
      }

      void prints_l(array_ptr<const char> str, size_t str_len ) {
         if ( !ignore ) {
            context.console_append(string(str, str_len));
         }
      }

      void printi(int64_t val) {
         if ( !ignore ) {
            context.console_append(val);
         }
      }

      void printui(uint64_t val) {
         if ( !ignore ) {
            context.console_append(val);
         }
      }

      void printi128(const __int128& val) {
         if ( !ignore ) {
            bool is_negative = (val < 0);
            unsigned __int128 val_magnitude;

            if( is_negative )
               val_magnitude = static_cast<unsigned __int128>(-val); // Works even if val is at the lowest possible value of a int128_t
            else
               val_magnitude = static_cast<unsigned __int128>(val);

            fc::uint128_t v(val_magnitude>>64, static_cast<uint64_t>(val_magnitude) );

            if( is_negative ) {
               context.console_append("-");
            }

            context.console_append(fc::variant(v).get_string());
         }
      }

      void printui128(const unsigned __int128& val) {
         if ( !ignore ) {
            fc::uint128_t v(val>>64, static_cast<uint64_t>(val) );
            context.console_append(fc::variant(v).get_string());
         }
      }

      void printsf( float val ) {
         if ( !ignore ) {
            // Assumes float representation on native side is the same as on the WASM side
            auto& console = context.get_console_stream();
            auto orig_prec = console.precision();

            console.precision( std::numeric_limits<float>::digits10 );
            context.console_append(val);

            console.precision( orig_prec );
         }
      }

      void printdf( double val ) {
         if ( !ignore ) {
            // Assumes double representation on native side is the same as on the WASM side
            auto& console = context.get_console_stream();
            auto orig_prec = console.precision();

            console.precision( std::numeric_limits<double>::digits10 );
            context.console_append(val);

            console.precision( orig_prec );
         }
      }

      void printqf( const float128_t& val ) {
         /*
          * Native-side long double uses an 80-bit extended-precision floating-point number.
          * The easiest solution for now was to use the Berkeley softfloat library to round the 128-bit
          * quadruple-precision floating-point number to an 80-bit extended-precision floating-point number
          * (losing precision) which then allows us to simply cast it into a long double for printing purposes.
          *
          * Later we might find a better solution to print the full quadruple-precision floating-point number.
          * Maybe with some compilation flag that turns long double into a quadruple-precision floating-point number,
          * or maybe with some library that allows us to print out quadruple-precision floating-point numbers without
          * having to deal with long doubles at all.
          */

         if ( !ignore ) {
            auto& console = context.get_console_stream();
            auto orig_prec = console.precision();

            console.precision( std::numeric_limits<long double>::digits10 );

            extFloat80_t val_approx;
            f128M_to_extF80M(&val, &val_approx);
            context.console_append( *(long double*)(&val_approx) );

            console.precision( orig_prec );
         }
      }

      void printn(const name& value) {
         if ( !ignore ) {
            context.console_append(value.to_string());
         }
      }

      void printhex(array_ptr<const char> data, size_t data_len ) {
         if ( !ignore ) {
            context.console_append(fc::to_hex(data, data_len));
         }
      }

   private:
      bool ignore;
};

class system_api : public context_aware_api {
   public:
      using context_aware_api::context_aware_api;

      uint64_t current_time() {
          return static_cast<uint64_t>(context.db.head_block_time().sec_since_epoch());
      }

      /*
      uint64_t publication_time() {
         return static_cast<uint64_t>( context.trx_context.published.time_since_epoch().count() );
      }
      */

};

REGISTER_INJECTED_INTRINSICS(call_depth_api,
   (call_depth_assert,  void()               )
);

REGISTER_INTRINSICS(console_api,
   (prints,                void(int)      )
   (prints_l,              void(int, int) )
   (printi,                void(int64_t)  )
   (printui,               void(int64_t)  )
   (printi128,             void(int)      )
   (printui128,            void(int)      )
   (printsf,               void(float)    )
   (printdf,               void(double)   )
   (printqf,               void(int)      )
   (printn,                void(int64_t)  )
   (printhex,              void(int, int) )
);

REGISTER_INTRINSICS(system_api,
        (current_time, int64_t()       )
        // (publication_time,   int64_t() )
);

std::istream& operator>>(std::istream& in, wasm_interface::vm_type& runtime) {
   std::string s;
   in >> s;
   if (s == "wavm")
      runtime = graphene::chain::wasm_interface::vm_type::wavm;
   else if (s == "binaryen")
      runtime = graphene::chain::wasm_interface::vm_type::binaryen;
   else
      in.setstate(std::ios_base::failbit);
   return in;
}

} } /// graphene::chain
