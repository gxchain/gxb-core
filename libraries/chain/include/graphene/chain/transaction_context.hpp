#pragma once

namespace graphene { namespace chain {

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

      private:
        friend class apply_context;

      private:
        bool is_initialized = false;
   };

} }
