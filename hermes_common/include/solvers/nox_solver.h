// This file is part of HermesCommon
//
// Copyright (c) 2009 hp-FEM group at the University of Nevada, Reno (UNR).
// Email: hpfem-group@unr.edu, home page: http://hpfem.org/.
//
// Hermes2D is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published
// by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// Hermes2D is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Hermes2D; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
/*! \file nox_solver.h
\brief NOX (nonliner) solver interface.
*/
#ifndef __HERMES_COMMON_NOX_SOLVER_H_
#define __HERMES_COMMON_NOX_SOLVER_H_

#include "linear_solver.h"
#include "nonlinear_solver.h"
#include "epetra.h"
#if (defined HAVE_NOX && defined HAVE_EPETRA && defined HAVE_TEUCHOS)
#include <NOX.H>
#ifdef _POSIX_C_SOURCE
# undef _POSIX_C_SOURCE	// pyconfig.h included by NOX_Epetra defines it
#endif
#ifdef _XOPEN_SOURCE
# undef _XOPEN_SOURCE	// pyconfig.h included by NOX_Epetra defines it
#endif
#include <NOX_Epetra.H>

namespace Hermes {
  namespace Solvers {

    /// \brief discrete problem uses in NOX solver
    /// Implents interfaces needed by NOX Epetra
    template <typename Scalar>
    class HERMES_API NoxDiscreteProblem : public NOX::Epetra::Interface::Required, 
      public NOX::Epetra::Interface::Jacobian,
      public NOX::Epetra::Interface::Preconditioner
    {
    public: 
      NoxDiscreteProblem(DiscreteProblemInterface<Scalar> * problem);

      void set_precond(Teuchos::RCP<Precond<Scalar> > &pc);
      /// \brief Accessor for preconditioner.
      Teuchos::RCP<Precond<Scalar> > get_precond() { return precond; }

      // that is generated by the Interface class.
      EpetraMatrix<Scalar> *get_jacobian() { return &jacobian; }

      /// \brief Compute and return F.
      virtual bool computeF(const Epetra_Vector &x, Epetra_Vector &f, FillType flag = Residual);

      /// \brief Compute an explicit Jacobian.
      virtual bool computeJacobian(const Epetra_Vector &x, Epetra_Operator &op);

      /// \brief Computes a user supplied preconditioner based on input vector x.
      /// @return true if computation was successful.
      virtual bool computePreconditioner(const Epetra_Vector &x, Epetra_Operator &m,
        Teuchos::ParameterList *precParams = 0);

    private:
      DiscreteProblemInterface<Scalar> * dp;
      /// \brief Jacobian (optional). \todo k cemu to je a kde se to pouziva
      EpetraMatrix<Scalar> jacobian;
      /// \brief Preconditioner (optional). \todo proc to je to tady
      Teuchos::RCP<Precond<Scalar> > precond; 
    };

    /// \brief Encapsulation of NOX nonlinear solver.
    /// \note complex numbers is not implemented yet
    /// @ingroup solvers
    template <typename Scalar>
    class HERMES_API NoxSolver : public NonlinearSolver<Scalar>
    {
    private: 
      NoxDiscreteProblem<Scalar> ndp;
    public:
      /// Constructor.
      NoxSolver(DiscreteProblemInterface<Scalar> *problem);

      virtual ~NoxSolver();

      virtual bool solve(Scalar* coeff_vec);

      virtual int get_num_iters() { return num_iters; }
      virtual double get_residual()  { return residual; }
      int get_num_lin_iters() { return num_lin_iters; }
      double get_achieved_tol()  { return achieved_tol; }

      /// setting of output flags.
      /// \param flags[in] output flags, sum of NOX::Utils::MsgType enum
      /// \par
      ///  %Error = 0, Warning = 0x1, OuterIteration = 0x2, InnerIteration = 0x4, 
      ///  Parameters = 0x8, Details = 0x10, OuterIterationStatusTest = 0x20, LinearSolverDetails = 0x40, 
      ///  TestDetails = 0x80, StepperIteration = 0x0100, StepperDetails = 0x0200, StepperParameters = 0x0400, 
      ///  Debug = 0x01000
      void set_output_flags(int flags) { output_flags = flags; }

      /// \name linear solver setters
      ///@{ 
      
      ///Determine the iterative technique used in the solve. The following options are valid:
      /// - "GMRES" - Restarted generalized minimal residual (default). 
      /// - "CG" - Conjugate gradient. 
      /// - "CGS" - Conjugate gradient squared. 
      /// - "TFQMR" - Transpose-free quasi-minimal reasidual. 
      /// - "BiCGStab" - Bi-conjugate gradient with stabilization. 
      /// - "LU" - Sparse direct solve (single processor only).
      void set_ls_type(const char *type) { ls_type = type; }
      /// maximum number of iterations in the linear solve. 
      void set_ls_max_iters(int iters) { ls_max_iters = iters; }
      /// Tolerance used by AztecOO to determine if an iterative linear solve has converged. 
      void set_ls_tolerance(double tolerance) { ls_tolerance = tolerance; }
      /// When using restarted GMRES this sets the maximum size of the Krylov subspace.
      void set_ls_sizeof_krylov_subspace(int size) { ls_sizeof_krylov_subspace = size; }
      ///@}

      /// \name convergence params
      /// @{
      
      /// Type of norm
      /// - NOX::Abstract::Vector::OneNorm \f[ \|x\| = \sum_{i=1}^n \| x_i \| \f]
      /// - NOX::Abstract::Vector::TwoNorm \f[ \|x\| = \sqrt{\sum_{i=1}^n \| x_i^2 \|} \f]
      /// - NOX::Abstract::Vector::MaxNorm \f[ \|x\| = \max_i \| x_i \| \f]
      void set_norm_type(NOX::Abstract::Vector::NormType type)  { conv.norm_type = type; }
      /// Determines whether to scale the norm by the problem size.
      /// - Scaled - scale
      /// - Unscaled - don't scale
      void set_scale_type(NOX::StatusTest::NormF::ScaleType type)  { conv.stype = type; }
      /// Maximum number of nonlinear solver iterations.
      void set_conv_iters(int iters)        { conv.max_iters = iters; }
      /// Absolute tolerance.
      void set_conv_abs_resid(double resid) { conv_flag.absresid = 1; conv.abs_resid = resid; }
      /// Relatice tolerance (scaled by initial guess).
      void set_conv_rel_resid(double resid) { conv_flag.relresid = 1; conv.rel_resid = resid; }
      /// disable absolute tolerance \todo is this needed?
      void disable_abs_resid() { conv_flag.absresid = 0; }
      /// disable relative tolerance \todo is this needed?
      void disable_rel_resid() { conv_flag.relresid = 0; }
      /// Update (change of solution) tolerance.
      void set_conv_update(double update)   { conv_flag.update = 1; conv.update = update; }
      /// Convergence test based on the weighted root mean square norm of the solution update between iterations.
      /// \param[in] rtol denotes the relative error tolerance, specified via rtol in the constructor
      /// \param[in] atol denotes the absolution error tolerance, specified via atol in the constructor.
      void set_conv_wrms(double rtol, double atol) 
      {
        conv_flag.wrms = 1;
        conv.wrms_rtol = rtol;
        conv.wrms_atol = atol;
      }

      virtual void set_precond(Teuchos::RCP<Precond<Scalar> > &pc);
      virtual void set_precond(const char *pc);

    protected:
      int num_iters;
      double residual;
      int num_lin_iters;
      double achieved_tol;  
      const char *nl_dir;

      int output_flags;
      const char *ls_type;
      int ls_max_iters;
      double ls_tolerance;
      int ls_sizeof_krylov_subspace;

      const char* precond_type;

      // convergence params
      struct conv_t {
        int max_iters;
        double abs_resid;
        double rel_resid;
        NOX::Abstract::Vector::NormType norm_type;
        NOX::StatusTest::NormF::ScaleType stype;
        double update;
        double wrms_rtol;
        double wrms_atol;
      } conv;

      struct conv_flag_t {
        unsigned absresid:1;
        unsigned relresid:1;
        unsigned wrms:1;
        unsigned update:1;
      } conv_flag;
    };
  }
}
#endif
#endif
