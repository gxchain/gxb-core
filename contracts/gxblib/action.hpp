#pragma once
#include <gxblib/action.h>
#include <gxblib/datastream.hpp>
#include <gxblib/serialize.hpp>
#include <gxblib/asset.hpp>

#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/variadic/to_tuple.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/facilities/overload.hpp>

namespace graphene {

   template<typename T>
   T unpack_action_data() {
      constexpr size_t max_stack_buffer_size = 512;
      size_t size = action_data_size();
      char* buffer = (char*)( max_stack_buffer_size < size ? malloc(size) : alloca(size) );
      read_action_data( buffer, size );
      return unpack<T>( buffer, size );
   }


   /**
    * This is the packed representation of an action along with
    * meta-data about the authorization levels.
    */
   struct action {
      uint64_t                   account;
      action_name                name;
      bytes                      data;

      action() = default;

      /**
       *  @tparam T - the type of the action data
       *  @param a - name of the contract account
       *  @param n - name of the action
       *  @param value - will be serialized via pack into data
       */
      template <typename T>
      action(uint64_t a, action_name n, T &&value)
          : account(a)
          , name(n)
          , data(pack(std::forward<T>(value)))
      {
      }

      GXBLIB_SERIALIZE(action, (account)(name)(data))

      void send() const
      {
          auto serialize = pack(*this);
          ::send_inline(serialize.data(), serialize.size());
      }

   };

} // namespace graphene

#define INLINE_ACTION_SENDER2( CONTRACT_CLASS, NAME )\
INLINE_ACTION_SENDER3( CONTRACT_CLASS, NAME, ::graphene::string_to_name(#NAME) )

#define INLINE_ACTION_SENDER(...) BOOST_PP_OVERLOAD(INLINE_ACTION_SENDER,__VA_ARGS__)(__VA_ARGS__)

#define SEND_INLINE_ACTION( CONTRACT, NAME, ... )\
INLINE_ACTION_SENDER(std::decay_t<decltype(CONTRACT)>, NAME)( (CONTRACT).get_self(),\
BOOST_PP_TUPLE_ENUM(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__)) );
