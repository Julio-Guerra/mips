#include <thread>
#include <iostream>
#include <fstream>
#include <atomic>
#include <mutex>

#include "pipeline.hh"

/// cout mutex
std::mutex mutex;

/// IF/ID Latch Example
/// IF produces the instruction to be consumed and decoded by the ID stage
struct IFIDLatch
{
    std::string instruction;
};

class stage
{
  public:
    stage(p::pipeline& p)
      : p_ (p)
    {
    }

  protected:
    p::pipeline& p_;
};

class IFStage : public stage
{
  public:
    IFStage(p::pipeline& p, const std::string& file)
      : stage (p), im_ (file)
    {
    }

    void* operator()(void* sr)
    {
      IFIDLatch* latch = nullptr;
      std::string instruction;

      std::getline(im_, instruction);
      if (im_.rdstate() == im_.goodbit)
      {
        mutex.lock();
        std::cout << "IF " << p_.t_get() << ": " << instruction << std::endl;
        mutex.unlock();

        latch = new IFIDLatch
          {
            .instruction = instruction
          };

        return latch;
      }
      else
        return p::latch::terminate;
    }

  private:
    std::ifstream im_;
};

class IDStage : public stage
{
  public:
    IDStage(p::pipeline& p)
      : stage (p)
    {
    }

    void* operator()(void* sr)
    {
      IFIDLatch* l = static_cast<IFIDLatch*>(sr);

      mutex.lock();
      std::cout << "ID " << p_.t_get() << ": " << l->instruction << std::endl;
      mutex.unlock();

      // example of
      // using internal pipeline noop latch instead of actually handling it in       // the stages. it could be useful at the beginning of your development
      // but may not work with your data units since **using this
      // special latch value results in *not calling* the stage functions
      // consuming it**. It will depend on your code.
      return l->instruction == "nop" ? p::latch::noop : l;
    }
};

class EXStage : public stage
{
  public:
    EXStage(p::pipeline& p)
      : stage (p)
    {
    }

    void* operator()(void* sr)
    {
      IFIDLatch* l = static_cast<IFIDLatch*>(sr);

      mutex.lock();
      std::cout << "EX " << p_.t_get() << ": " << l->instruction << std::endl;
      mutex.unlock();

      return l;
    }
};

class MEMStage : public stage
{
  public:
    MEMStage(p::pipeline& p)
      : stage (p)
    {
    }

    void* operator()(void* sr)
    {
      IFIDLatch* l = static_cast<IFIDLatch*>(sr);

      mutex.lock();
      std::cout << "MEM " << p_.t_get() << ": " << l->instruction << std::endl;
      mutex.unlock();

      return l;
    }
};

class WBStage : public stage
{
  public:
    WBStage(p::pipeline& p)
      : stage (p)
    {
    }

    void* operator()(void* sr)
    {
      IFIDLatch* l = static_cast<IFIDLatch*>(sr);

      mutex.lock();
      std::cout << "WB " << p_.t_get() << ": " << l->instruction << std::endl;
      mutex.unlock();

      delete l;
      return l;
    }
};

int main(int argc, char* argv[])
{
  p::pipeline p;

  IFStage ifs(p, argv[1]);
  IDStage ids(p);
  EXStage exs(p);
  MEMStage mems(p);
  WBStage wbs(p);

  p.add_stage(std::ref(ifs));
  p.add_stage(std::ref(ids));
  p.add_stage(std::ref(exs));
  p.add_stage(std::ref(mems));
  p.add_stage(std::ref(wbs));

  p.run();

  std::cout
    << std::endl
    << "Total Number of Cycles = " << p.t_get() - 1 << std::endl
    << "Average IPC = TODO" << std::endl;

  return 0;
}
