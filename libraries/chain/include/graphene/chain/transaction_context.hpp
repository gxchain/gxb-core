#pragma once

namespace graphene { namespace chain {

   namespace config {
       const fc::microseconds cpu_duration_limit = fc::microseconds(10 * 1000);//default 10ms TODO can config by configuration
   }
   
   class transaction_context {
      private:
         void init();

      public:
        transaction_context();

        void init_for_implicit_trx();

        void init_for_input_trx();

        void exec();
        void finalize();
        void squash();

        void checktime() const;

        void pause_billing_timer();
        void resume_billing_timer();

        void add_ram_usage();

        uint64_t get_cpu_usage() const
        {
            return transaction_cpu_usage_us;
        }

      private:
        void validate_cpu_usage_to_bill() const;

      private:
        friend class apply_context;
        
      private:
        bool                    is_initialized = false;
        fc::time_point          start;
        fc::time_point          _deadline;
        mutable uint64_t        transaction_cpu_usage_us;
   };
} }
