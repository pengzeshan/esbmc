#include <util/cache.h>
#include <util/message.h>
#include <utility>
#include <util/crypto_hash.h>

void assertion_cache::run_on_assert(symex_target_equationt::SSA_stept &step)
{
  ++total;
  auto [it, ins] = db.emplace(std::make_pair(step.cond, step.guard));
  if(!ins)
  {
    ++hits;
    step.ignore = true;
  }
}

bool assertion_cache::run(symex_target_equationt::SSA_stepst &eq)
{
  fine_timet algorithm_start = current_time();
  for(auto &step : eq)
    run_on_step(step);
  fine_timet algorithm_stop = current_time();
  log_status(
    "Caching time: {}s (removed {}/{} assertions)",
    time2string(algorithm_stop - algorithm_start),
    hits,
    total);
  return true;
}
