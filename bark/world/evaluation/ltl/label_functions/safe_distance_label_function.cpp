// Copyright (c) 2020 fortiss GmbH
//
// Authors: Julian Bernhard, Klemens Esterle, Patrick Hart and
// Tobias Kessler
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "safe_distance_label_function.hpp"
#include "bark/world/observed_world.hpp"

namespace bark {
namespace world {
namespace evaluation {

using bark::models::dynamic::StateDefinition;

SafeDistanceLabelFunction::SafeDistanceLabelFunction(
    const std::string& label_str, bool to_rear, double delta, double a_e,
    double a_o, bool consider_crossing_corridors,
    unsigned int max_agents_for_crossing)
    : BaseLabelFunction(label_str),
      to_rear_(to_rear),
      delta_(delta),
      a_e_(a_e),
      a_o_(a_o),
      consider_crossing_corridors_(consider_crossing_corridors),
      max_agents_for_crossing_(max_agents_for_crossing) {}

LabelMap SafeDistanceLabelFunction::Evaluate(const world::ObservedWorld& observed_world) const {
  if(!EvaluateEgoCorridor(observed_world)) {
    return {{GetLabel(), false}};; 
  }
  bool safe_dist = true;
  if(consider_crossing_corridors_) {
    safe_dist = EvaluateCrossingCorridors(observed_world);
  }
  return {{GetLabel(), safe_dist}};
}


bool SafeDistanceLabelFunction::EvaluateEgoCorridor(
    const world::ObservedWorld& observed_world) const {
  auto ego = std::const_pointer_cast<Agent>(observed_world.GetEgoAgent());
  auto lane_corridor =
      ego->GetRoadCorridor()->GetNearestLaneCorridor(ego->GetCurrentPosition());
  auto fr_agents =
      observed_world.GetAgentFrontRearForId(ego->GetAgentId(), lane_corridor);
  bool distance_safe = true;
  if (to_rear_ && fr_agents.rear.first) {
    distance_safe = CheckSafeDistance(fr_agents.rear.first, ego,
                                      fr_agents.rear.second, a_o_, a_e_);
  } else if (fr_agents.front.first) {
    distance_safe = CheckSafeDistance(ego, fr_agents.front.first,
                                      fr_agents.front.second, a_e_, a_o_);
  }
  return distance_safe;
}

bool SafeDistanceLabelFunction::EvaluateCrossingCorridors(
  const world::ObservedWorld& observed_world) const {
  const auto ego_pos = observed_world.CurrentEgoPosition();
  const auto nearest_agents = observed_world.GetNearestAgents(ego_pos,
                          max_agents_for_crossing_ + 1); // one more since ego agent is included
  for (const auto nearest_agent : nearest_agents) {
    if(nearest_agent.first == observed_world.GetEgoAgentId()) continue;

    // Find this agents front and back agent
    auto lane_corridor =
        nearest_agent.second->GetRoadCorridor()
                ->GetNearestLaneCorridor(nearest_agent.second->GetCurrentPosition());
    auto fr_agents =
        observed_world.GetAgentFrontRearForId(
            nearest_agent.second->GetAgentId(), lane_corridor);

    // if front agent is ego agent, we check safety distance
    // (assuming that ego agent as rear agent is not unsafe
    // since ego is in different lane corridor)
    if(!fr_agents.front.first) continue;
    if(fr_agents.front.first->GetAgentId() != observed_world.GetEgoAgentId()) continue;
    bool distance_safe = CheckSafeDistance(nearest_agent.second, fr_agents.front.first,
                                    fr_agents.front.second, a_o_, a_e_);
    if(!distance_safe) return distance_safe;
  }
  return true;
}

bool SafeDistanceLabelFunction::CheckSafeDistance(
    const AgentPtr& rear_agent, const AgentPtr& front_agent,
    const FrenetPosition& frenet_dist, const double a_r,
    const double a_f) const {
  double dist = std::abs(frenet_dist.lon) - front_agent->GetShape().rear_dist_ -
                rear_agent->GetShape().front_dist_;
  if (dist < 0.0) {
    VLOG(2) << "Vehicle distance between agent " << rear_agent->GetAgentId()
            << " and " << front_agent->GetAgentId() << " = " << dist
            << " < 0! Skipping!";
    return true;
  }

  double v_f = front_agent->GetCurrentState()(StateDefinition::VEL_POSITION);
  double v_r = rear_agent->GetCurrentState()(StateDefinition::VEL_POSITION);
  double v_f_star = CalcVelFrontStar(v_f, a_f);
  double t_stop_f_star = -v_f_star / a_r;
  double t_stop_r = -v_r / a_r;
  double t_stop_f = -v_f / a_f;

  bool distance_safe;
  double safe_dist_0 = CalcSafeDistance0(v_r, a_r);
  double safe_dist_1 = CalcSafeDistance1(v_r, v_f, a_r, a_f);
  double safe_dist_2 = CalcSafeDistance2(v_r, v_f, a_r, a_f);
  double safe_dist_3 = CalcSafeDistance3(v_r, v_f, a_r, a_f);

  if (dist > safe_dist_0 || (delta_ <= t_stop_f && dist > safe_dist_3)) {
    distance_safe = true;
  } else if (delta_ <= t_stop_f && a_f > a_r && v_f_star < v_r &&
             t_stop_r < t_stop_f_star) {
    distance_safe = dist > safe_dist_2;
  } else {
    distance_safe = dist > safe_dist_1;
  }
  return distance_safe;
}

double SafeDistanceLabelFunction::CalcVelFrontStar(const double v_f,
                                                   const double a_f) const {
  // see Theorem 4 in "Formalising and Monitoring Traffic Rules for Autonomous
  // Vehicles in Isabelle/HOL"
  double v_f_star;
  double t_stop_f = -v_f / a_f;
  if (delta_ <= t_stop_f) {
    v_f_star = v_f + a_f * delta_;
  } else {
    v_f_star = 0.0;
  }
  return v_f_star;
}

double SafeDistanceLabelFunction::CalcSafeDistance0(const double v_r,
                                                    const double a_r) const {
  return v_r * delta_ - pow(v_r, 2) / (2.0 * a_r);
}

double SafeDistanceLabelFunction::CalcSafeDistance1(const double v_r,
                                                    const double v_f,
                                                    const double a_r,
                                                    const double a_f) const {
  return v_r * delta_ - pow(v_r, 2) / (2.0 * a_r) + pow(v_f, 2) / (2.0 * a_f);
}

double SafeDistanceLabelFunction::CalcSafeDistance2(const double v_r,
                                                    const double v_f,
                                                    const double a_r,
                                                    const double a_f) const {
  double sqrt_numerator = v_f + a_f * delta_ - v_r;
  return pow(sqrt_numerator, 2) / (2.0 * (a_f - a_r)) - v_f * delta_ -
         0.5 * a_f * pow(delta_, 2) + v_r * delta_;
}

double SafeDistanceLabelFunction::CalcSafeDistance3(const double v_r,
                                                    const double v_f,
                                                    const double a_r,
                                                    const double a_f) const {
  return v_r * delta_ - pow(v_r, 2) / (2.0 * a_r) - v_f * delta_ -
         a_f * pow(delta_, 2) / 2.0;
}

}  // namespace evaluation
}  // namespace world
}  // namespace bark
