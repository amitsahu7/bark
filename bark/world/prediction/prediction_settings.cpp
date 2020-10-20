// Copyright (c) 2020 fortiss GmbH
//
// Authors: Julian Bernhard, Klemens Esterle, Patrick Hart and
// Tobias Kessler
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "bark/world/prediction/prediction_settings.hpp"
#include "bark/models/behavior/not_started/behavior_not_started.hpp"
#include "bark/world/observed_world.hpp"

namespace bark {
namespace world {
namespace prediction {

using bark::models::behavior::BehaviorModelPtr;
using bark::models::behavior::BehaviorNotStarted;
using models::behavior::BehaviorStatus;

PredictionSettings::PredictionSettings(
    const BehaviorModelPtr& ego_prediction_model,
    const BehaviorModelPtr& default_prediction_model,
    const BehaviorModelPtr& specific_prediction_model,
    const std::vector<AgentId>& specific_prediction_agents)
    : ego_prediction_model_(ego_prediction_model),
      specific_prediction_model_(specific_prediction_model),
      default_prediction_model_(default_prediction_model),
      specific_prediction_agents_(specific_prediction_agents.begin(),
                                  specific_prediction_agents.end()) {}

PredictionSettings::PredictionSettings(
    const BehaviorModelPtr& ego_prediction_model,
    const BehaviorModelPtr& default_prediction_model)
    : PredictionSettings(ego_prediction_model, default_prediction_model,
                         nullptr, {}) {}

void PredictionSettings::ApplySettings(
    bark::world::ObservedWorld& observed_world) const {
  observed_world.SetEgoBehaviorModel(
      BehaviorModelPtr(ego_prediction_model_->Clone()));

  if (default_prediction_model_) {
    for (const auto& agent : observed_world.GetOtherAgents()) {
      if (agent.second->GetBehaviorStatus() == BehaviorStatus::VALID &&
          agent.second->IsValidAtTime(observed_world.GetWorldTime())) {
        // make sure this agent shall not be controlled by
        // specific_prediction_model
        if (specific_prediction_agents_.count(agent.first) == 0) {
          observed_world.SetBehaviorModel(
              agent.first,
              BehaviorModelPtr(default_prediction_model_->Clone()));
        }
      } else {
        // no model available, use BehaviorNotStarted
        observed_world.SetBehaviorModel(
            agent.first, std::make_shared<BehaviorNotStarted>(nullptr));
      }
    }
  }
  if (specific_prediction_model_) {
    for (const auto& agent_id : specific_prediction_agents_) {
      observed_world.SetBehaviorModel(
          agent_id, BehaviorModelPtr(specific_prediction_model_->Clone()));
    }
  }
}
}  // namespace prediction
}  // namespace world
}  // namespace bark
