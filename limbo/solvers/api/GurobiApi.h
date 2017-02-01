/**
 * @file   GurobiApi.h
 * @brief  Gurobi API by writing out problem file in LP format and read solution. 
 *         Consider the file IO overhead before using it. 
 * @author Yibo Lin
 * @date   Nov 2014
 */

#ifndef _LIMBO_SOLVERS_API_GUROBIAPI_H
#define _LIMBO_SOLVERS_API_GUROBIAPI_H

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <boost/lexical_cast.hpp>
#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/assert.hpp> 
// make sure gurobi is configured properly 
#include "gurobi_c++.h"

/// namespace for Limbo
namespace limbo 
{ 
/// namespace for Solvers 
namespace solvers 
{

using std::cout;
using std::endl;
using std::string;
using std::vector;
using std::list;
using std::pair;
using std::make_pair;
using boost::int32_t;
using boost::shared_ptr;

/// This api needs a file in LP format as input.
/// And then read the solution output by gurobi 
/// I plan to come up with more efficient api without file io operations 
template <typename T>
struct GurobiFileApi 
{
    /// @brief value type 
	typedef T value_type;

	/// @brief data structure to save solution
	struct solution_type 
	{
		value_type obj; ///< objective value 
		list<pair<string, value_type> > vVariable; ///< list of (variable, solution) pairs 
	};
	/// @brief API function 
    /// @param fileName input file name 
    /// @return solution 
	virtual shared_ptr<solution_type> operator()(string const& fileName, bool = true) const 
	{
		// better to use full path for file name 
		shared_ptr<solution_type> pSol (new solution_type);
		// remove previous solution file 
		cout << "rm -rf "+fileName+".sol" << endl;
		cout << system(("rm -rf "+fileName+".sol").c_str()) << endl;;

		cout << "solve linear program "+fileName << endl;
		this->solve_lp(fileName);

		// read rpt 
		{
			std::ifstream solFile ((fileName+".sol").c_str(), std::ifstream::in);
			if (!solFile.good()) BOOST_ASSERT_MSG(false, ("failed to open " + fileName + ".sol").c_str());

			string var;
			double value;

			// read objective value 
			solFile >> var >> var >> var >> var >> value;
			pSol->obj = value;

			while (!solFile.eof())
			{
				solFile >> var >> value;
				pSol->vVariable.push_back(make_pair(var, value));
			}
			solFile.close();
		}

		return pSol;
	}
	/// Core function to solve lp problem with Gurobi. 
	/// It is modified from examples of Gurobi. 
	/// Basically it reads input problem file, and output solution file. 
    /// @param fileName input file 
	virtual void solve_lp(string fileName) const 
	{
		try 
		{
			GRBEnv env = GRBEnv();
			GRBModel model = GRBModel(env, fileName+".lp");

			model.optimize();

			int optimstatus = model.get(GRB_IntAttr_Status);

			if (optimstatus == GRB_INF_OR_UNBD) 
			{
				model.getEnv().set(GRB_IntParam_Presolve, 0);
				model.optimize();
				optimstatus = model.get(GRB_IntAttr_Status);
			}

			if (optimstatus == GRB_OPTIMAL) 
			{
				double objval = model.get(GRB_DoubleAttr_ObjVal);
				cout << "Optimal objective: " << objval << endl;
				// write result 
				model.write(fileName+".sol");
			} 
			else if (optimstatus == GRB_INFEASIBLE) 
			{
				cout << "Model is infeasible" << endl;

				// compute and write out IIS

				model.computeIIS();
				model.write(fileName+".ilp");
			} 
			else if (optimstatus == GRB_UNBOUNDED) 
			{
				cout << "Model is unbounded" << endl;
			} 
			else 
			{
				cout << "Optimization was stopped with status = "
					<< optimstatus << endl;
			}
		} 
		catch(GRBException e) 
		{
			cout << "Error code = " << e.getErrorCode() << endl;
			cout << e.getMessage() << endl;
		} 
		catch (...) 
		{
			cout << "Error during optimization" << endl;
		}
	}

};

} // namespace solvers
} // namespace limbo

#endif 
