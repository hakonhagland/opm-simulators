/*
  Copyright 2013, 2015 SINTEF ICT, Applied Mathematics.
  Copyright 2014 STATOIL ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OPM_BLACKOILPOLYMERMODEL_HEADER_INCLUDED
#define OPM_BLACKOILPOLYMERMODEL_HEADER_INCLUDED

#include <cassert>

#include <opm/autodiff/AutoDiffBlock.hpp>
#include <opm/autodiff/AutoDiffHelpers.hpp>
#include <opm/autodiff/BlackoilPropsAdInterface.hpp>
#include <opm/autodiff/LinearisedBlackoilResidual.hpp>
#include <opm/autodiff/NewtonIterationBlackoilInterface.hpp>
#include <opm/polymer/PolymerProperties.hpp>
#include <opm/polymer/fullyimplicit/PolymerPropsAd.hpp>

#include <array>

struct UnstructuredGrid;
struct Wells;

namespace Opm {

    namespace parameter { class ParameterGroup; }
    class DerivedGeology;
    class RockCompressibility;
    class NewtonIterationBlackoilInterface;
    class PolymerBlackoilState;
    class WellStateFullyImplicitBlackoilPolymer;


    /// A model implementation for three-phase black oil with polymer.
    ///
    /// The simulator is capable of handling three-phase problems
    /// where gas can be dissolved in oil and vice versa, with polymer
    /// in the water phase. It uses an industry-standard TPFA
    /// discretization with per-phase upwind weighting of mobilities.
    ///
    /// It uses automatic differentiation via the class AutoDiffBlock
    /// to simplify assembly of the jacobian matrix.
    template<class Grid>
    class BlackoilPolymerModel
    {
    public:

        // ---------  Types and enums  ---------

        typedef AutoDiffBlock<double> ADB;
        typedef ADB::V V;
        typedef ADB::M M;
        typedef PolymerBlackoilState ReservoirState;
        typedef WellStateFullyImplicitBlackoilPolymer WellState;

        /// Model-specific solver parameters.
        struct ModelParameter
        {
            double dp_max_rel_;
            double ds_max_;
            double dr_max_rel_;
            double max_residual_allowed_;
            double tolerance_mb_;
            double tolerance_cnv_;
            double tolerance_wells_;

            ModelParameter( const parameter::ParameterGroup& param );
            ModelParameter();

            void reset();
        };

        // ---------  Public methods  ---------

        /// Construct the model. It will retain references to the
        /// arguments of this functions, and they are expected to
        /// remain in scope for the lifetime of the solver.
        /// \param[in] param            parameters
        /// \param[in] grid             grid data structure
        /// \param[in] fluid            fluid properties
        /// \param[in] geo              rock properties
        /// \param[in] rock_comp_props  if non-null, rock compressibility properties
        /// \param[in] wells            well structure
        /// \param[in] linsolver        linear solver
        /// \param[in] has_disgas       turn on dissolved gas
        /// \param[in] has_vapoil       turn on vaporized oil feature
        /// \param[in] has_polymer      turn on polymer feature
        /// \param[in] terminal_output  request output to cout/cerr
        BlackoilPolymerModel(const ModelParameter&          param,
                             const Grid&                     grid ,
                             const BlackoilPropsAdInterface& fluid,
                             const DerivedGeology&           geo  ,
                             const RockCompressibility*      rock_comp_props,
                             const PolymerPropsAd&           polymer_props_ad,
                             const Wells*                    wells,
                             const NewtonIterationBlackoilInterface& linsolver,
                             const bool has_disgas,
                             const bool has_vapoil,
                             const bool has_polymer,
                             const bool terminal_output);

        /// \brief Set threshold pressures that prevent or reduce flow.
        /// This prevents flow across faces if the potential
        /// difference is less than the threshold. If the potential
        /// difference is greater, the threshold value is subtracted
        /// before calculating flow. This is treated symmetrically, so
        /// flow is prevented or reduced in both directions equally.
        /// \param[in]  threshold_pressures_by_face   array of size equal to the number of faces
        ///                                   of the grid passed in the constructor.
        void setThresholdPressures(const std::vector<double>& threshold_pressures_by_face);

        /// Called once before each time step.
        /// \param[in] dt                     time step size
        /// \param[in, out] reservoir_state   reservoir state variables
        /// \param[in, out] well_state        well state variables
        void prepareStep(const double dt,
                         ReservoirState& reservoir_state,
                         WellState& well_state);

        /// Called once after each time step.
        /// \param[in] dt                     time step size
        /// \param[in, out] reservoir_state   reservoir state variables
        /// \param[in, out] well_state        well state variables
        void afterStep(const double dt,
                       ReservoirState& reservoir_state,
                       WellState& well_state);

        /// Assemble the residual and Jacobian of the nonlinear system.
        /// \param[in]      reservoir_state   reservoir state variables
        /// \param[in, out] well_state        well state variables
        /// \param[in]      initial_assembly  pass true if this is the first call to assemble() in this timestep
        void assemble(const ReservoirState& reservoir_state,
                      WellState& well_state,
                      const bool initial_assembly);

        /// \brief Compute the residual norms of the mass balance for each phase,
        /// the well flux, and the well equation.
        /// \return a vector that contains for each phase the norm of the mass balance
        /// and afterwards the norm of the residual of the well flux and the well equation.
        std::vector<double> computeResidualNorms() const;

        /// The size (number of unknowns) of the nonlinear system of equations.
        int sizeNonLinear() const;

        /// Number of linear iterations used in last call to solveJacobianSystem().
        int linearIterationsLastSolve() const;

        /// Solve the Jacobian system Jx = r where J is the Jacobian and
        /// r is the residual.
        V solveJacobianSystem() const;

        /// Apply an update to the primary variables, chopped if appropriate.
        /// \param[in]      dx                updates to apply to primary variables
        /// \param[in, out] reservoir_state   reservoir state variables
        /// \param[in, out] well_state        well state variables
        void updateState(const V& dx,
                         ReservoirState& reservoir_state,
                         WellState& well_state);

        /// Return true if output to cout is wanted.
        bool terminalOutput() const;

        /// Compute convergence based on total mass balance (tol_mb) and maximum
        /// residual mass balance (tol_cnv).
        /// \param[in]   dt          timestep length
        /// \param[in]   iteration   current iteration number
        bool getConvergence(const double dt, const int iteration);

        /// The number of active phases in the model.
        int numPhases() const;

    private:

        // ---------  Types and enums  ---------

        typedef Eigen::Array<double,
                             Eigen::Dynamic,
                             Eigen::Dynamic,
                             Eigen::RowMajor> DataBlock;

        struct ReservoirResidualQuant {
            ReservoirResidualQuant();
            std::vector<ADB> accum; // Accumulations
            ADB              mflux; // Mass flux (surface conditions)
            ADB              b;     // Reciprocal FVF
            ADB              head;  // Pressure drop across int. interfaces
            ADB              mob;   // Phase mobility (per cell)
        };

        struct SolutionState {
            SolutionState(const int np);
            ADB              pressure;
            ADB              temperature;
            std::vector<ADB> saturation;
            ADB              rs;
            ADB              rv;
            ADB              concentration;
            ADB              qs;
            ADB              bhp;
            // Below are quantities stored in the state for optimization purposes.
            std::vector<ADB> canonical_phase_pressures; // Always has 3 elements, even if only 2 phases active.
        };

        struct WellOps {
            WellOps(const Wells* wells);
            M w2p;              // well -> perf (scatter)
            M p2w;              // perf -> well (gather)
        };

        enum { Water        = BlackoilPropsAdInterface::Water,
               Oil          = BlackoilPropsAdInterface::Oil  ,
               Gas          = BlackoilPropsAdInterface::Gas  ,
               MaxNumPhases = BlackoilPropsAdInterface::MaxNumPhases
         };

        enum PrimalVariables { Sg = 0, RS = 1, RV = 2 };

        // ---------  Data members  ---------

        const Grid&         grid_;
        const BlackoilPropsAdInterface& fluid_;
        const DerivedGeology&           geo_;
        const RockCompressibility*      rock_comp_props_;
        const PolymerPropsAd&           polymer_props_ad_;
        const Wells*                    wells_;
        const NewtonIterationBlackoilInterface&    linsolver_;
        // For each canonical phase -> true if active
        const std::vector<bool>         active_;
        // Size = # active phases. Maps active -> canonical phase indices.
        const std::vector<int>          canph_;
        const std::vector<int>          cells_;  // All grid cells
        HelperOps                       ops_;
        const WellOps                   wops_;
        V                               cmax_;
        const bool has_disgas_;
        const bool has_vapoil_;
        const bool has_polymer_;
        const int  poly_pos_;

        ModelParameter                 param_;
        bool use_threshold_pressure_;
        V threshold_pressures_by_interior_face_;

        std::vector<ReservoirResidualQuant> rq_;
        std::vector<PhasePresence> phaseCondition_;
        V well_perforation_pressure_diffs_; // Diff to bhp for each well perforation.

        LinearisedBlackoilResidual residual_;

        /// \brief Whether we print something to std::cout
        bool terminal_output_;

        std::vector<int>         primalVariable_;
        V pvdt_;

        // ---------  Private methods  ---------

        // return true if wells are available
        bool wellsActive() const { return wells_ ? wells_->number_of_wells > 0 : false ; }
        // return wells object
        const Wells& wells () const { assert( bool(wells_ != 0) ); return *wells_; }

        SolutionState
        constantState(const PolymerBlackoilState& x,
                      const WellStateFullyImplicitBlackoilPolymer& xw) const;

        void
        makeConstantState(SolutionState& state) const;

        SolutionState
        variableState(const PolymerBlackoilState& x,
                      const WellStateFullyImplicitBlackoilPolymer& xw) const;

        void
        computeAccum(const SolutionState& state,
                     const int            aix  );

        void computeWellConnectionPressures(const SolutionState& state,
                                            const WellStateFullyImplicitBlackoilPolymer& xw);

        void
        addWellControlEq(const SolutionState& state,
                         const WellStateFullyImplicitBlackoilPolymer& xw,
                         const V& aliveWells);

        void
        addWellEq(const SolutionState& state,
                  WellStateFullyImplicitBlackoilPolymer& xw,
                  V& aliveWells);

        void updateWellControls(WellStateFullyImplicitBlackoilPolymer& xw) const;

        std::vector<ADB>
        computePressures(const SolutionState& state) const;

        std::vector<ADB>
        computePressures(const ADB& po,
                         const ADB& sw,
                         const ADB& so,
                         const ADB& sg) const;

        V
        computeGasPressure(const V& po,
                           const V& sw,
                           const V& so,
                           const V& sg) const;

        std::vector<ADB>
        computeRelPerm(const SolutionState& state) const;

        void
        computeMassFlux(const int               actph ,
                        const V&                transi,
                        const ADB&              kr    ,
                        const ADB&              p     ,
                        const SolutionState&    state );

        void
        computeCmax(PolymerBlackoilState& state);

        ADB
        computeMc(const SolutionState& state) const;

        void applyThresholdPressures(ADB& dp);

        ADB
        fluidViscosity(const int               phase,
                       const ADB&              p    ,
                       const ADB&              temp ,
                       const ADB&              rs   ,
                       const ADB&              rv   ,
                       const std::vector<PhasePresence>& cond,
                       const std::vector<int>& cells) const;

        ADB
        fluidReciprocFVF(const int               phase,
                         const ADB&              p    ,
                         const ADB&              temp ,
                         const ADB&              rs   ,
                         const ADB&              rv   ,
                         const std::vector<PhasePresence>& cond,
                         const std::vector<int>& cells) const;

        ADB
        fluidDensity(const int               phase,
                     const ADB&              p    ,
                     const ADB&              temp ,
                     const ADB&              rs   ,
                     const ADB&              rv   ,
                     const std::vector<PhasePresence>& cond,
                     const std::vector<int>& cells) const;

        V
        fluidRsSat(const V&                p,
                   const V&                so,
                   const std::vector<int>& cells) const;

        ADB
        fluidRsSat(const ADB&              p,
                   const ADB&              so,
                   const std::vector<int>& cells) const;

        V
        fluidRvSat(const V&                p,
                   const V&                so,
                   const std::vector<int>& cells) const;

        ADB
        fluidRvSat(const ADB&              p,
                   const ADB&              so,
                   const std::vector<int>& cells) const;

        ADB
        poroMult(const ADB& p) const;

        ADB
        transMult(const ADB& p) const;

        void
        classifyCondition(const SolutionState&        state,
                          std::vector<PhasePresence>& cond ) const;

        const std::vector<PhasePresence>
        phaseCondition() const {return phaseCondition_;}

        void
        classifyCondition(const PolymerBlackoilState&        state);


        /// update the primal variable for Sg, Rv or Rs. The Gas phase must
        /// be active to call this method.
        void
        updatePrimalVariableFromState(const PolymerBlackoilState&        state);

        /// Update the phaseCondition_ member based on the primalVariable_ member.
        void
        updatePhaseCondFromPrimalVariable();

        /// \brief Compute the reduction within the convergence check.
        /// \param[in] B     A matrix with MaxNumPhases columns and the same number rows
        ///                  as the number of cells of the grid. B.col(i) contains the values
        ///                  for phase i.
        /// \param[in] tempV A matrix with MaxNumPhases columns and the same number rows
        ///                  as the number of cells of the grid. tempV.col(i) contains the
        ///                   values
        ///                  for phase i.
        /// \param[in] R     A matrix with MaxNumPhases columns and the same number rows
        ///                  as the number of cells of the grid. B.col(i) contains the values
        ///                  for phase i.
        /// \param[out] R_sum An array of size MaxNumPhases where entry i contains the sum
        ///                   of R for the phase i.
        /// \param[out] maxCoeff An array of size MaxNumPhases where entry i contains the
        ///                   maximum of tempV for the phase i.
        /// \param[out] B_avg An array of size MaxNumPhases where entry i contains the average
        ///                   of B for the phase i.
        /// \param[in]  nc    The number of cells of the local grid.
        /// \return The total pore volume over all cells.
        double
        convergenceReduction(const Eigen::Array<double, Eigen::Dynamic, MaxNumPhases+1>& B,
                             const Eigen::Array<double, Eigen::Dynamic, MaxNumPhases+1>& tempV,
                             const Eigen::Array<double, Eigen::Dynamic, MaxNumPhases+1>& R,
                             std::array<double,MaxNumPhases+1>& R_sum,
                             std::array<double,MaxNumPhases+1>& maxCoeff,
                             std::array<double,MaxNumPhases+1>& B_avg,
                             int nc) const;

        double dpMaxRel() const { return param_.dp_max_rel_; }
        double dsMax() const { return param_.ds_max_; }
        double drMaxRel() const { return param_.dr_max_rel_; }
        double maxResidualAllowed() const { return param_.max_residual_allowed_; }

    };
} // namespace Opm

#include "BlackoilPolymerModel_impl.hpp"


#endif // OPM_BLACKOILPOLYMERMODEL_HEADER_INCLUDED
