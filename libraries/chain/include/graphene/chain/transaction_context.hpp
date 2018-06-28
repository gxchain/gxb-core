#pragma once

namespace graphene { namespace chain {

   namespace config {
       const fc::microseconds cpu_duration_limit = fc::microseconds(10 * 1000);//default 10ms TODO can config by configuration
   }

   class transaction_context {
      private:
         void init();

      public:

        void init_for_implicit_trx();

        void init_for_input_trx();
        transaction_context(database &d);

        void exec();
        void finalize();
        void squash();

        void checktime() const;

        uint64_t get_cpu_usage() const
        {
            return transaction_cpu_usage_us;
        }

      private:
        void validate_cpu_usage_to_bill() const;

      private:
        void dispatch_action(const action &a, account_name receiver);
        inline void dispatch_action(const action &a)
        {
            dispatch_action(a, a.account);
        };

      private:
        friend class apply_context;

      public:
        database &db() const { assert(_db); return *_db; }

      private:
        database                *_db;
        fc::time_point          start;
        fc::time_point          _deadline;
        mutable uint64_t        transaction_cpu_usage_us;
   };
} }
