// Copyright (c) 2020 fortiss GmbH
//
// Authors: Julian Bernhard, Klemens Esterle, Patrick Hart and
// Tobias Kessler
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "bark/world/observed_world.hpp"
#include "bark/world/evaluation/safe_distances/evaluator_static_safe_dist.hpp"

namespace bark {
namespace world {
namespace evaluation {

using bark::geometry::Point2d;
using bark::geometry::Polygon;
using bark::geometry::Pose;
using bark::models::dynamic::State;
using bark::models::dynamic::StateDefinition;

EvaluatorStaticSafeDist::EvaluatorStaticSafeDist(const bark::commons::ParamsPtr& params, const AgentId& agent_id) :
   EvaluatorSafeDistBase(agent_id),
   lateral_safety_dist_(params->GetReal("EvaluatorStaticSafeDist::LateralSafeDist",
        "Minimum required lateral distance", 0.5f)),
   longitudinal_safety_dist_(params->GetReal("EvaluatorStaticSafeDist::LongitudinalSafeDist",
         "Minimum required longitudinal distance", 0.5f)) {}

bool EvaluatorStaticSafeDist::CheckSafeDistance(const world::ObservedWorld& observed_world) {
  bool colliding = false;
  int num_agents = 4;
  auto ego_agent = observed_world.GetEgoAgent();
  if (!ego_agent) {
    return false;
  }
  State ego_state = ego_agent->GetCurrentState();
  Point2d ego_position(ego_state(X_POSITION), ego_state(Y_POSITION));
  AgentMap nearby_agents = observed_world.GetNearestAgents(ego_position, num_agents);

  // Evaluation assumes that - at zero orientation - shape of ego agent is oriented such that lateral 
  // coordinate is y and longitudinal coordinate is x
  Pose agent_pose(ego_state(StateDefinition::X_POSITION),
                  ego_state(StateDefinition::Y_POSITION),
                  ego_state(StateDefinition::THETA_POSITION));
  const auto ego_scaled_shape = std::dynamic_pointer_cast<Polygon>(
          ego_agent->GetShape().Scale(longitudinal_safety_dist_, lateral_safety_dist_)->Transform(agent_pose));

  for (const auto& agent : nearby_agents) {
    if (observed_world.GetEgoAgentId() != agent.second->GetAgentId()) {
      const Polygon& agent_polygon =
          agent.second->GetPolygonFromState(agent.second->GetCurrentState());
      if (Collide(*ego_scaled_shape, agent_polygon)) {
        colliding = true;
        break;
      }
    }
  }
  return !colliding;
}



}  // namespace evaluation
}  // namespace world
}  // namespace bark
