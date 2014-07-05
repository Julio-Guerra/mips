#ifndef PIPELINE_HH_
# define PIPELINE_HH_

# include <atomic>
# include <vector>
# include <functional>
# include <thread>

namespace p
{
  ///
  /// Latch Result.
  /// Producer/Consumer wrapper of a single element.
  /// Lock the producer while the element has not been consumed.
  /// Lock the consumer while the element has not been produced.
  /// The no-op latch is used at startup to fill-in the pipeline.
  /// The pipeline is initialized with noop latches to make it run
  /// and be filled by a generator stage: the first one.
  ///
  class latch
  {
    public:
      ///
      /// Special latch values.
      /// - noop:
      ///     don't call the stage's function and produce a noop.
      /// - something:
      ///     used to fill the pipeline at start-up as latch result
      ///     of last pipeline stage and consummed by the first stage
      ///     at start-up.
      /// - terminate:
      ///     to be returned to stop the pipeline flushing it.
      /// \{
      static void* noop;
      static void* something;
      static void* terminate;
      /// \}

      void* noop_latch;

      ///
      /// Ctor.
      /// Internal no-op state at construction.
      /// Used to bootstrap the pipeline with noops. The pipeline is
      /// initialized with noop latches but the first one with random data
      /// for the first cycles and which is the generator of the pipeline.
      ///
      latch()
        : noop_latch (noop), latch_ (noop)
      {
        produced_ = true;
      }

      ///
      /// Move Ctor.
      /// std::atomic member => non-trivial move constructor.
      ///
      latch(latch&& l)
        : noop_latch (l.noop_latch),
          latch_ (l.latch_),
          produced_ (l.produced_.load())
      {
      }

      void* consume()
      {
        void* l;

        while (!produced_)
          ;
        l = latch_;
        produced_ = false;

        return l;
      }

      void produce(void* latch)
      {
        while (produced_)
          ;

        latch_ = latch;
        produced_ = true;
      }

      inline void latch_set(void* latch)
      {
        latch_ = latch;
      }

    private:
      void*             latch_;
      std::atomic<bool> produced_;
  };
  void* latch::noop = (void*) 0x1;
  void* latch::something = (void*) 0x2;
  void* latch::terminate = nullptr;

  class spinlock
  {
    public:
      spinlock()
        : lock_ (ATOMIC_FLAG_INIT)
      {
        release();
      }

      inline void acquire()
      {
        while (lock_.test_and_set())
          ;
      }

      inline void release()
      {
        lock_.clear();
      }

    private:
      std::atomic_flag lock_;
  };

  ///
  /// Userland SMP barrier !
  ///
  class barrier
  {
    public:
      barrier()
        : left_ (0), lock_ (), event_ (false), reset_ (0)
      {
      }

      void init(unsigned int nb_threads)
      {
        reset_ = nb_threads;
        left_ = nb_threads;
        event_ = false;
      }

      inline void leave()
      {
        --reset_;
        auto left = --left_;
        if (left == 0)
          event_ = true;
      }

      void operator()()
      {
        lock_.acquire();
        auto left = --left_;
        if (left == 0)
          event_ = true;
        else
        {
          lock_.release();
          while (event_ == false)
            ;
        }

        if (++left_ == reset_)
        {
          event_ = false;
          lock_.release();
        }
      }

    private:
      std::atomic<unsigned int> left_;
      spinlock                  lock_;
      std::atomic<bool>         event_;
      volatile unsigned int     reset_;
  };

  ///
  /// Software Pipeline with the following properties:
  ///   - 1 thread per stage
  ///   - there is no runahead stage, they all run at the same clock rate:
  ///     the pipeline clock cycle is the same at any given moment in every
  ///     stages. This makes data-forwarding between stages much easier by
  ///     simply spinlocking on a value while it is not yet produced.
  ///   - the first stage consumes the last stage's output data, thus
  ///     making every stage running at the worst stage execution time.
  ///
  class pipeline
  {
    public:
      typedef std::function<void*(void*)> stage_type;

      ///
      /// Push back the pipeline a new stage.
      /// A stage is any callable element.
      ///
      pipeline& add_stage(stage_type s)
      {
        size_t index = p_.size();
        size_t pred = index - 1;

        if (index == 0)
          pred = 0;
        else
          p_[0].pred = index;

        p_.emplace_back(p_.size(), pred, s);

        return *this;
      }

      ///
      /// Current Cycle getter.
      ///
      inline unsigned int t_get() const
      {
        return t_;
      }

      void run()
      {
        size_t nb_stages = p_.size();

        if (nb_stages == 0)
          return;

        barrier_.init(nb_stages);
        // every stage is latched with a noop but the first one which
        // will insert data into the pipeline
        p_[nb_stages - 1].out.latch_set(latch::something);
        p_[nb_stages - 1].out.noop_latch = latch::something;

        // create and start the stage's threads
        std::vector<std::thread> threads;
        for (auto& s : p_)
          threads.emplace_back(std::ref(*this), std::ref(s));

        for (size_t i = 0; i < nb_stages; ++i)
          if (threads[i].joinable())
            threads[i].join();
      }

      ///
      /// Internal Stage Descriptor.
      ///
      typedef struct sd
      {
          sd(size_t i, size_t p, stage_type& s)
            : index (i), pred (p), f (s), out ()
          {
          }

          /// Stage index in the pipeline.
          size_t        index;

          /// N-1 Stage index in the pipeline.
          size_t        pred;

          ///
          /// Stage core function.
          /// Latch result of stage index-1 given as argument.
          ///
          stage_type&   f;

          /// Latch result of the stage's function.
          latch         out;
      } sd_type;

      void operator()(sd_type& this_stage)
      {
        unsigned int t = 0;
        auto& pred = p_[this_stage.pred];

        do
        {
          void* latch = pred.out.consume();

          // Synchronization barrier insure every stages are the exact same
          // clock cycle
          // Is it possible to remove the barrier and find a producer/consumer
          // constraint ensuring this property? As it is (and without the
          // barrier), there can be two stages at different clock cycles at the
          // same time in the pipeline... The "consume previous stage result and
          // produce the next stage's result". There might be a barrier-free
          // solution.
          barrier_();

          ++t;
          t_ = t;

          if (latch == latch::noop)
            this_stage.out.produce(this_stage.out.noop_latch);
          else
          {
            if (latch == latch::terminate)
            {
              barrier_.leave();
              this_stage.out.produce(latch::terminate);
              break;
            }

            latch = this_stage.f(latch);

            this_stage.out.produce(latch);

            if (latch == latch::terminate)
            {
              barrier_.leave();
              // consume pred's data until it gets the terminate latch
              // otherwise no one would consume it...
              while (pred.out.consume() != latch::terminate)
                ;
              break;
            }
          }
        } while (true);
      }

    private:
      std::atomic<unsigned int> t_;
      barrier                   barrier_;
      std::vector<sd_type>      p_;
  };
}

#endif /* !PIPELINE_HH_ */
