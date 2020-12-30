// Copyright (c) 2020 fortiss GmbH
//
// Authors: Julian Bernhard, Klemens Esterle, Patrick Hart and
// Tobias Kessler
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "bark/world/evaluation/evaluator_planning_time.hpp"


namespace bark {
namespace world {
class World;
namespace evaluation {
using bark::models::behavior::BehaviorModelPtr;

EvaluationReturn EvaluatorPlanningTime::Evaluate(const world::World& world) {
  auto ego_agent = world.GetAgent(this->agent_id_);
  double planning_time = -1e3;
  if (ego_agent) {
    auto model = ego_agent->GetBehaviorModel();
    planning_time = model->GetLastSolutionTime();
  }
  return planning_time;
}

EvaluationReturn EvaluatorPlanningTime::Evaluate(
    const world::ObservedWorld& observed_world) {
  int num_agents = 4;
  auto ego_agent = observed_world.GetEgoAgent();
  BehaviorModelPtr model = ego_agent->GetBehaviorModel();
  double planning_time = model->GetLastSolutionTime();
  return planning_time;
}

}  // namespace evaluation
}  // namespace world
}  // namespace bark
