#include "SparseSetAlternatives/tcc.h"
namespace tcc
{

EndGameFlags Tcc::runAndDeleteApplication(tcc::Application *app)
{
  EndGameFlags flags = app->run();
  delete app;
  return flags;
}

} // namespace tcc
