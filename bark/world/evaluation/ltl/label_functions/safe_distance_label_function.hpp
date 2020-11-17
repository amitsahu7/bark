// Copyright (c) 2020 fortiss GmbH
//
// Authors: Julian Bernhard, Klemens Esterle, Patrick Hart and
// Tobias Kessler
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#ifndef BARK_WORLD_EVALUATION_LTL_LABELS_SAFE_DISTANCE_LABEL_FUNCTION_HPP_
#define BARK_WORLD_EVALUATION_LTL_LABELS_SAFE_DISTANCE_LABEL_FUNCTION_HPP_

#include <string>
#include <vector>

#include "bark/world/evaluation/ltl/label_functions/base_label_function.hpp"
#include "bark/commons/transformation/frenet_state.hpp"
#include "bark/world/objects/agent.hpp"
#include "bark/world/objects/object.hpp"

namespace bark {
namespace world {
namespace evaluation {

using bark::commons::transformation::FrenetPosition;
using bark::world::objects::AgentPtr;

class SafeDistanceLabelFunction : public BaseLabelFunction {
 public:
  SafeDistanceLabelFunction(const std::string& label_str, bool to_rear,
                            double delta, double a_e, double a_o,
                            bool consider_crossing_corridors = false,
                            unsigned int max_agents_for_crossing = 4);
  LabelMap Evaluate(const world::ObservedWorld& observed_world) const override;

  bool EvaluateEgoCorridor(const world::ObservedWorld& observed_world) const;
  bool EvaluateCrossingCorridors(const world::ObservedWorld& observed_world) const;

  bool IsOncomingVehicle(const bark::world::objects::AgentPtr& front_agent,
           const bark::world::objects::AgentPtr& rear_agent) const;

  bool GetToRear() const { return to_rear_; }
  double GetDelta() const { return delta_; }
  double GetMaxDecelEgo() const { return a_e_; }
  double GetMaxDecelOther() const { return a_o_; }

 private:
  bool CheckSafeDistance(
    const float v_f, const float v_r, const float dist,
    const double a_r,  const double a_f) const;
  inline double CalcVelFrontStar(double v_f, double a_f) const;
  inline double CalcSafeDistance0(double v_r, double a_r) const;
  inline double CalcSafeDistance1(double v_r, double v_f, double a_r,
                                  double a_f) const;
  inline double CalcSafeDistance2(double v_r, double v_f, double a_r,
                                  double a_f) const;
  inline double CalcSafeDistance3(double v_r, double v_f, double a_r,
                                  double a_f) const;

  bool to_rear_;
  double delta_;  //! Reaction time
  double a_e_;    //! Max. deceleration of ego
  double a_o_;    //! Max. deceleration of front/rear agent (why rear agent?)
  bool consider_crossing_corridors_;
  unsigned int max_agents_for_crossing_;
};

}  // namespace evaluation
}  // namespace world
}  // namespace bark

#endif  // BARK_WORLD_EVALUATION_LTL_LABELS_SAFE_DISTANCE_LABEL_FUNCTION_HPP_
