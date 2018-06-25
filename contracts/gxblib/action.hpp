#pragma once
#include <gxblib/action.h>
#include <gxblib/datastream.hpp>
#include <gxblib/serialize.hpp>

#include <boost/preprocessor/variadic/size.hpp>
#include <boost/preprocessor/variadic/to_tuple.hpp>
#include <boost/preprocessor/tuple/enum.hpp>
#include <boost/preprocessor/facilities/overload.hpp>

namespace graphene {

   /**
    * @defgroup actioncppapi Action C++ API
    * @ingroup actionapi
    * @brief Type-safe C++ wrapers for Action C API
    *
    * @note There are some methods from the @ref actioncapi that can be used directly from C++
    *
    * @{
    */

   /**
    *
    *  This method unpacks the current action at type T.
    *
    *  @brief Interpret the action body as type T
    *
    *  Example:
    *  @code
    *  struct dummy_action {
    *    char a; //1
    *    unsigned long long b; //8
    *    int  c; //4
    *
    *    GXBLIB_SERIALIZE( dummy_action, (a)(b)(c) )
    *  };
    *  dummy_action msg = unpack_action_data<dummy_action>();
    *  @endcode
    */
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
       *  @tparam Action - a type derived from action_meta<Scope,Name>
       *  @param value - will be serialized via pack into data
       */
      template<typename Action>
      action(const Action& value ) {
         account       = Action::get_account();
         name          = Action::get_name();
         data          = pack(value);
      }


      /**
       *  @tparam T - the type of the action data
       *  @param a - name of the contract account
       *  @param n - name of the action
       *  @param value - will be serialized via pack into data
       */
      template<typename T>
      action( uint64_t a, action_name n, T&& value )
      :account(a), name(n), data(pack(std::forward<T>(value))) {}

      GXBLIB_SERIALIZE( action, (account)(name)(data) )

      /**
       * Retrieve the unpacked data as T
       * @tparam T expected type of data
       * @return the action data
       */
      template<typename T>
      T data_as() {
         gxb_assert( name == T::get_name(), "Invalid name" );
         gxb_assert( account == T::get_account(), "Invalid account" );
         return unpack<T>( &data[0], data.size() );
      }

   };

   template<uint64_t Account, uint64_t Name>
   struct action_meta {
      static uint64_t get_account() { return Account; }
      static uint64_t get_name()  { return Name; }
   };


 ///@} actioncpp api

} // namespace graphene

#define INLINE_ACTION_SENDER2( CONTRACT_CLASS, NAME )\
INLINE_ACTION_SENDER3( CONTRACT_CLASS, NAME, ::graphene::string_to_name(#NAME) )

#define INLINE_ACTION_SENDER(...) BOOST_PP_OVERLOAD(INLINE_ACTION_SENDER,__VA_ARGS__)(__VA_ARGS__)

#define SEND_INLINE_ACTION( CONTRACT, NAME, ... )\
INLINE_ACTION_SENDER(std::decay_t<decltype(CONTRACT)>, NAME)( (CONTRACT).get_self(),\
BOOST_PP_TUPLE_ENUM(BOOST_PP_VARIADIC_SIZE(__VA_ARGS__), BOOST_PP_VARIADIC_TO_TUPLE(__VA_ARGS__)) );


#define ACTION( CODE, NAME ) struct NAME : ::graphene::action_meta<CODE, ::graphene::string_to_name(#NAME) >
