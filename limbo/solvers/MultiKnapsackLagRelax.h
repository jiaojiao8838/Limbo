/**
 * @file   MultiKnapsackLagRelax.h
 * @author Yibo Lin
 * @date   Feb 2017
 * @brief  Solve multiple knapsack problem with lagrangian relaxation 
 */
#ifndef LIMBO_SOLVERS_MULTIKNAPSACKLAGRELAX_H
#define LIMBO_SOLVERS_MULTIKNAPSACKLAGRELAX_H

#include <cmath>
#include <limbo/solvers/Solvers.h>

/// namespace for Limbo 
namespace limbo 
{
/// namespace for Limbo.Solvers 
namespace solvers 
{

// forward declaration 
template <typename T>
class LagMultiplierUpdater;
template <typename T>
class SubGradientDescent; 
template <typename T, typename V>
class ProblemScaler; 
template <typename T, typename V>
class L2NormScaler; 
template <typename T, typename V>
class FeasibleSearcher;
template <typename T, typename V>
class SearchByAdjustCoefficient;

/// @brief Solve multiple knapsack problem with lagrangian relaxation 
/// 
/// Suppose we have a set of item \f$C\f$ and a set of knapsack \f$B\f$. 
/// Use \f$x_{ij}\f$ to denote item \f$i\f$ is assigned to knapsack \f$j\f$. 
/// The primal problem \f$P\f$ is as follows, \n
/// \f{eqnarray*}{
/// & min. & \sum_{i,j} c_{ij} \cdot x_{ij}, \\
/// & s.t. & \sum_{i} a_i x_{ij} \le b_j, \forall j \in B,  \\
/// &      & \sum_{j} x_{ij} = 1, \forall i \in C, \\
/// &      & x_{ij} \in \{0, 1\}, \forall i \in C, j \in B.  
/// \f}
/// \n
/// 
/// The procedure to solve the problem is to iteratively solve following 
/// lagrangian subproblem \f$L\f$, 
/// \f{eqnarray*}{
/// & min. & \sum_{i,j} c_{ij} \cdot x_{ij} + \sum_{j} \lambda_j (\sum_{i} a_i x_{ij} - b_j), \\
/// & s.t. & \sum_{j} x_{ij} = 1, \forall i \in C, \\
/// &      & x_{ij} \in \{0, 1\}, \forall i \in C, j \in B.  
/// \f}
/// \n
/// where the subproblem can be solved simply by sorting the weight of \f$x_{ij}\f$ 
/// and pick the ones with least cost in each iteration. 
/// However, it is difficult to check optimality conditions. 
/// So we track the evolution of capacity violations 
/// and stop once we observe no significant improvement. 
/// The rest violations are solved by heuristic approaches. 
/// 
/// @tparam T coefficient type 
/// @tparam V variable type 
template <typename T, typename V>
class MultiKnapsackLagRelax
{
    public:
        /// @brief linear model type for the problem 
        typedef LinearModel<T, V> model_type; 
        /// @nowarn
        typedef typename model_type::coefficient_value_type coefficient_value_type; 
        typedef typename model_type::variable_value_type variable_value_type; 
        typedef typename model_type::variable_type variable_type;
        typedef typename model_type::constraint_type constraint_type; 
        typedef typename model_type::expression_type expression_type; 
        typedef typename model_type::term_type term_type; 
        typedef typename model_type::property_type property_type;
        typedef LagMultiplierUpdater<coefficient_value_type> updater_type; 
        typedef ProblemScaler<coefficient_value_type, variable_value_type> scaler_type; 
        typedef FeasibleSearcher<coefficient_value_type, variable_value_type> searcher_type; 
        /// @endnowarn
        
        /// @brief constructor 
        /// @param model pointer to the model of problem 
        MultiKnapsackLagRelax(model_type* model);
        /// @brief copy constructor 
        /// @param rhs right hand side 
        MultiKnapsackLagRelax(MultiKnapsackLagRelax const& rhs);
        /// @brief assignment
        /// @param rhs right hand side 
        MultiKnapsackLagRelax& operator=(MultiKnapsackLagRelax const& rhs);
        /// @brief destructor 
        ~MultiKnapsackLagRelax(); 
        
        /// @brief API to run the algorithm 
        /// @param updater an object to update lagrangian multipliers, use default updater if NULL  
        /// @param scaler an object to scale constraints and objective, use default scaler if NULL 
        /// @param searcher an object to search for feasible solutions, use default searcher if NULL 
        SolverProperty operator()(updater_type* updater = NULL, scaler_type* scaler = NULL, searcher_type* searcher = NULL); 
        /// @brief kernel lagrangian iterations 
        /// @param updater an object to update lagrangian multipliers
        /// @param beginIter begin iteration number 
        /// @param endIter end iteration number 
        SolverProperty solveSubproblems(updater_type* updater, unsigned int beginIter, unsigned int endIter);

        /// @return maximum iterations 
        unsigned int maxIterations() const;
        /// @brief set maximum iterations 
        /// @param maxIter maximum iterations 
        void setMaxIterations(unsigned int maxIter);
        /// @return flag of whether use initial solutions 
        bool useInitialSolutions() const;
        /// @brief set whether use initial solutions 
        /// @param f flag 
        void setUseInitialSolutions(bool f); 

        /// @name print functions for debug 
        ///@{
        /// @brief print variable groups to file 
        /// @param filename output file name 
        /// @return true if succeed 
        bool printVariableGroup(std::string const& filename) const; 
        /// @brief print variable groups 
        /// @param os output stream 
        /// @return output stream 
        std::ostream& printVariableGroup(std::ostream& os = std::cout) const; 
        /// @brief print coefficients of variables in objective to file 
        /// @param filename output file name 
        /// @return true if succeed 
        bool printObjCoef(std::string const& filename) const; 
        /// @brief print coefficients of variables in objective  
        /// @param os output stream 
        /// @return output stream 
        std::ostream& printObjCoef(std::ostream& os = std::cout) const; 
        /// @brief print lagrangian multipliers to file 
        /// @param filename output file name 
        /// @return true if succeed 
        bool printLagMultiplier(std::string const& filename) const; 
        /// @brief print lagrangian multipliers 
        /// @param os output stream 
        /// @return output stream 
        std::ostream& printLagMultiplier(std::ostream& os = std::cout) const; 
        ///@}

        /// @brief Predicate whether a constraint is a capacity constraint 
        struct CapacityConstraintPred
        {
            std::vector<constraint_type> const& vConstraint; ///< constraints 

            /// @brief constructor 
            /// @param v array of constraints 
            CapacityConstraintPred(std::vector<constraint_type> const& v) : vConstraint(v) {}

            /// @return true if \constr is a capacity constraint 
            /// @param constr constraint 
            bool operator()(constraint_type const& constr) const 
            {
                return constr.sense() != '='; 
            }
            /// @return true if constraint is a capacity constraint 
            /// @param id constraint index 
            bool operator()(unsigned int id) const 
            {
                return vConstraint[id].sense() != '='; 
            }
        };

        /// @brief Predicate to sort variables according to their coefficient from small to large 
        struct CompareVariableByCoefficient
        {
            std::vector<coefficient_value_type> const& vObjCoef; ///< coefficients in objective for comparison 

            /// @brief constructor 
            /// @param v array of coefficients in objective 
            CompareVariableByCoefficient(std::vector<coefficient_value_type> const& v) 
                : vObjCoef(v)
            {
            }

            /// @param v1 variable 
            /// @param v2 variable 
            /// @return true if \v1 has smaller coefficient than \v2 
            bool operator()(variable_type const& v1, variable_type const& v2) const 
            {
                return vObjCoef[v1.id()] < vObjCoef[v2.id()]; 
            }
        };

    protected:
        /// @brief copy object 
        void copy(MultiKnapsackLagRelax const& rhs);
        /// @brief kernel function to solve the problem 
        /// @param updater an object to update lagrangian multipliers
        /// @param scaler an object to scale constraints and objective, use default scaler if NULL 
        /// @param searcher an object to search for feasible solutions, use default searcher if NULL 
        SolverProperty solve(updater_type* updater, scaler_type* scaler, searcher_type* searcher);
        /// @brief scale problem for better numerical instability 
        /// @param scaler an object to scale constraints and objective, use default scaler if NULL 
        void scale(scaler_type* scaler); 
        /// @brief recover problem from scaling 
        void unscale(); 
        /// @brief prepare weights of variables in objective 
        /// and classify constraints by marking capacity constraints and single item constraints 
        void prepare();
        /// @brief update lagrangian multipliers 
        /// @param updater an object to update lagrangian multipliers
        void updateLagMultipliers(updater_type* updater); 
        /// @brief solve lagrangian subproblem 
        void solveLag(); 
        /// @brief compute slackness in an iteration 
        void computeSlackness(); 
        /// @brief evaluate objective of the lagrangian subproblem 
        coefficient_value_type evaluateLagObjective() const;
        /// @brief check convergence of current solution 
        /// @return @ref limbo::solvers::SolverProperty OPTIMAL if converged; @ref limbo::solvers::SolverProperty  SUBOPTIMAL if a feasible solution found 
        SolverProperty converge(); 
        /// @brief post process solution if failed to converge to OPTIMAL after maximum iteration. 
        /// It choose the best feasible solutions in store 
        /// @param updater an object to update lagrangian multipliers
        /// @param status current status of solutions 
        /// @param searcher an object to search for feasible solutions 
        SolverProperty postProcess(updater_type* updater, searcher_type* searcher, SolverProperty status); 

        model_type* m_model; ///< model for the problem 

        std::vector<coefficient_value_type> m_vObjCoef; ///< coefficients variables in objective 
        std::vector<std::vector<variable_type> > m_vVariableGroup; ///< group variables according to item 
        std::vector<unsigned int> m_vConstraintPartition; ///< indices of constraints, the first partition is capacity constraints 
        std::vector<coefficient_value_type> m_vLagMultiplier; ///< array of lagrangian multipliers 
        std::vector<coefficient_value_type> m_vSlackness; ///< array of slackness values in each iteration, \f$ b-Ax \f$
        std::vector<coefficient_value_type> m_vScalingFactor; ///< scaling factor for constraints and objective, last entry is for objective
        std::vector<bool> m_vFixedGroup; ///< groups/items that are fixed, which means we will not change the assignment 
        coefficient_value_type m_objConstant; ///< constant value in objective from lagrangian relaxation
        coefficient_value_type m_lagObj; ///< current objective of the lagrangian subproblem 
        unsigned int m_iter; ///< current iteration 
        unsigned int m_maxIters; ///< maximum number of iterations 
        bool m_useInitialSol; ///< whether use initial solutions or not 

        std::vector<variable_value_type> m_vBestVariableSol; ///< best feasible solution found so far 
        coefficient_value_type m_bestObj; ///< best objective found so far 

    private:
        friend class FeasibleSearcher<coefficient_value_type, variable_value_type>; 
};

template <typename T, typename V>
MultiKnapsackLagRelax<T, V>::MultiKnapsackLagRelax(typename MultiKnapsackLagRelax<T, V>::model_type* model)
{
    // T must be a floating point number 
    limboStaticAssert(!std::numeric_limits<T>::is_integer);
    // V must be an integer number 
    limboStaticAssert(std::numeric_limits<V>::is_integer);

    m_model = model; 
    m_lagObj = 0; 
    m_objConstant = 0; 
    m_iter = 0; 
    m_maxIters = 1000; 
    m_bestObj = std::numeric_limits<coefficient_value_type>::max();
    m_useInitialSol = false; 
}
template <typename T, typename V>
MultiKnapsackLagRelax<T, V>::MultiKnapsackLagRelax(MultiKnapsackLagRelax<T, V> const& rhs)
{
    copy(rhs);
}
template <typename T, typename V>
MultiKnapsackLagRelax<T, V>& MultiKnapsackLagRelax<T, V>::operator=(MultiKnapsackLagRelax<T, V> const& rhs)
{
    if (this != &rhs)
        copy(rhs);
    return *this;
}
template <typename T, typename V>
MultiKnapsackLagRelax<T, V>::~MultiKnapsackLagRelax() 
{
}
template <typename T, typename V>
SolverProperty MultiKnapsackLagRelax<T, V>::operator()(typename MultiKnapsackLagRelax<T, V>::updater_type* updater, typename MultiKnapsackLagRelax<T, V>::scaler_type* scaler, typename MultiKnapsackLagRelax<T, V>::searcher_type* searcher) 
{
    return solve(updater, scaler, searcher);
}
template <typename T, typename V>
unsigned int MultiKnapsackLagRelax<T, V>::maxIterations() const 
{
    return m_maxIters;
}
template <typename T, typename V>
void MultiKnapsackLagRelax<T, V>::setMaxIterations(unsigned int maxIters) 
{
    m_maxIters = maxIters; 
}
template <typename T, typename V>
bool MultiKnapsackLagRelax<T, V>::useInitialSolutions() const 
{
    return m_useInitialSol;
}
template <typename T, typename V>
void MultiKnapsackLagRelax<T, V>::setUseInitialSolutions(bool f) 
{
    m_useInitialSol = f; 
}
template <typename T, typename V>
void MultiKnapsackLagRelax<T, V>::copy(MultiKnapsackLagRelax<T, V> const& rhs)
{
    m_model = rhs.m_model; 
    m_vObjCoef = rhs.m_vObjCoef; 
    m_vVariableGroup = rhs.m_vVariableGroup;
    m_vConstraintPartition = rhs.m_vConstraintPartition;
    m_vLagMultiplier = rhs.m_vLagMultiplier;
    m_vSlackness = rhs.m_vSlackness;
    m_objConstant = rhs.m_objConstant; 
    m_lagObj = rhs.m_lagObj; 
    m_iter = rhs.m_iter; 
    m_maxIters = rhs.m_maxIters;
    m_useInitialSol = rhs.m_useInitialSol; 

    m_vBestVariableSol = rhs.m_vBestVariableSol;
    m_bestObj = rhs.m_bestObj;
}
template <typename T, typename V>
SolverProperty MultiKnapsackLagRelax<T, V>::solve(typename MultiKnapsackLagRelax<T, V>::updater_type* updater, typename MultiKnapsackLagRelax<T, V>::scaler_type* scaler, typename MultiKnapsackLagRelax<T, V>::searcher_type* searcher)
{
    bool defaultUpdater = false; 
    bool defaultScaler = false; 
    bool defaultSearcher = false; 
    // use default updater if NULL 
    if (updater == NULL)
    {
        updater = new SubGradientDescent<coefficient_value_type>(); 
        defaultUpdater = true; 
    }
    // use default scaler if NULL 
    if (scaler == NULL)
    {
        scaler = new L2NormScaler<coefficient_value_type, variable_value_type>(); 
        defaultScaler = true; 
    }
    // use default searcher if NULL 
    if (searcher == NULL)
    {
        searcher = new SearchByAdjustCoefficient<coefficient_value_type, variable_value_type>(this); 
        defaultSearcher = true; 
    }

    // scale problem 
    scale(scaler); 
    // prepare data structure 
    prepare(); 

    // solve lagrangian subproblem 
    SolverProperty status = solveSubproblems(updater, 0, m_maxIters); 

    // try to find a feasible solution by post refinement 
    status = postProcess(updater, searcher, status); 

    // recover problem from scaling 
    unscale();

    // recycle default updater 
    if (defaultUpdater)
        delete updater; 
    // recycle default scaler 
    if (defaultScaler)
        delete scaler;
    // recycle default searcher 
    if (defaultSearcher)
        delete searcher; 

    return status;
}
template <typename T, typename V>
SolverProperty MultiKnapsackLagRelax<T, V>::solveSubproblems(typename MultiKnapsackLagRelax<T, V>::updater_type* updater, unsigned int beginIter, unsigned int endIter)
{
    // solve lagrangian subproblem 
    SolverProperty status = INFEASIBLE; 
    for (m_iter = beginIter; m_iter < endIter; ++m_iter)
    {
#if 1
        limboPrint(kDEBUG, "iteration %u\n", m_iter); 
#endif
        if (!useInitialSolutions() || m_iter != 0)
            solveLag(); 
        computeSlackness();
#ifdef DEBUG_MULTIKNAPSACKLAGRELAX
        printVariableGroup(std::cout);
        printObjCoef(std::cout);
        printLagMultiplier(std::cout);
#endif
        if ((status = converge()) == OPTIMAL || m_iter+1 == endIter)
            break; 

        updateLagMultipliers(updater);

    }

    return status; 
}
template <typename T, typename V>
void MultiKnapsackLagRelax<T, V>::scale(typename MultiKnapsackLagRelax<T, V>::scaler_type* scaler)
{
    // compute scaling factors and perform scale 
    m_vScalingFactor.resize(m_model->constraints().size()+1);
    // constraints 
    for (unsigned int i = 0, ie = m_model->constraints().size(); i < ie; ++i)
    {
        m_vScalingFactor[i] = scaler->operator()(m_model->constraints()[i]); 
        m_model->scaleConstraint(i, 1.0/m_vScalingFactor[i]);
    }
    // objective 
    m_vScalingFactor.back() = scaler->operator()(m_model->objective()); 
    m_model->scaleObjective(1.0/m_vScalingFactor.back());
}
template <typename T, typename V>
void MultiKnapsackLagRelax<T, V>::unscale()
{
    // constraints 
    for (unsigned int i = 0, ie = m_model->constraints().size(); i < ie; ++i)
        m_model->scaleConstraint(i, m_vScalingFactor[i]);
    // objective 
    m_model->scaleObjective(m_vScalingFactor.back());
}
template <typename T, typename V>
void MultiKnapsackLagRelax<T, V>::prepare() 
{
    // initialize weights of variables in objective 
    m_vObjCoef.assign(m_model->numVariables(), 0); 
    for (typename std::vector<term_type>::const_iterator it = m_model->objective().terms().begin(), ite = m_model->objective().terms().end(); it != ite; ++it)
        m_vObjCoef[it->variable().id()] += it->coefficient();

    // partition all capacity constraints to the beginning of the array 
    m_vConstraintPartition.resize(m_model->constraints().size()); 
    unsigned int i = 0; 
    for (unsigned int ie = m_model->constraints().size(); i < ie; ++i)
        m_vConstraintPartition[i] = i; 
    std::vector<unsigned int>::iterator bound = std::partition(m_vConstraintPartition.begin(), m_vConstraintPartition.end(), CapacityConstraintPred(m_model->constraints()));
    m_vLagMultiplier.assign(std::distance(m_vConstraintPartition.begin(), bound), 0); 
    m_vSlackness.assign(m_vLagMultiplier.size(), 0);

    // change the sense of '>' to '<'
    for (std::vector<unsigned int>::iterator it = m_vConstraintPartition.begin(); it != bound; ++it)
        m_model->constraints().at(*it).normalize('<');

    // group variables according items 
    // the variables for one item will be grouped 
    // I assume the rest constraints are all single item constraints 
    m_vVariableGroup.resize(std::distance(bound, m_vConstraintPartition.end()));
    m_vFixedGroup.assign(m_vVariableGroup.size(), false); 
    i = 0; 
    for (std::vector<unsigned int>::iterator it = bound, ite = m_vConstraintPartition.end(); it != ite; ++it, ++i)
    {
        constraint_type const& constr = m_model->constraints()[*it];
#ifdef DEBUG_MULTIKNAPSACKLAGRELAX
        limboAssert(constr.sense() == '='); 
#endif
        expression_type const& expr = constr.expression();
        for (typename std::vector<term_type>::const_iterator itt = expr.terms().begin(), itte = expr.terms().end(); itt != itte; ++itt)
        {
            m_vVariableGroup[i].push_back(itt->variable());
            // this item always assign to this bin 
            if (m_model->variableLowerBound(itt->variable())) 
                m_vFixedGroup[i] = true; 
        }
    }

}
template <typename T, typename V>
void MultiKnapsackLagRelax<T, V>::updateLagMultipliers(typename MultiKnapsackLagRelax<T, V>::updater_type* updater)
{
    // for each capacity constraint of bin 
    for (unsigned int i = 0, ie = m_vLagMultiplier.size(); i < ie; ++i)
    {
        constraint_type const& constr = m_model->constraints().at(m_vConstraintPartition[i]); 
        // get slackness with current solution 
        coefficient_value_type slackness = m_vSlackness[i];

        coefficient_value_type& multiplier = m_vLagMultiplier[i];

        // compute new lagrangian multiplier 
        coefficient_value_type newMultiplier = updater->operator()(m_iter, multiplier, slackness);
        coefficient_value_type deltaMultiplier = newMultiplier-multiplier; 

        // apply lagrangian multiplier to objective 
        // \f$ \sum_j \lambda_j (a_i x_{ij}) \f$
        for (typename std::vector<term_type>::const_iterator it = constr.expression().terms().begin(), ite = constr.expression().terms().end(); it != ite; ++it)
            m_vObjCoef[it->variable().id()] += it->coefficient()*deltaMultiplier;
        // \f$ \sum_j -\lambda_j b_j \f$
        m_objConstant += -constr.rightHandSide()*deltaMultiplier;

        // update multiplier 
        multiplier = newMultiplier; 
    }
}
template <typename T, typename V>
void MultiKnapsackLagRelax<T, V>::solveLag()
{
    // for each item 
    unsigned int i = 0; 
    for (typename std::vector<std::vector<variable_type> >::iterator it = m_vVariableGroup.begin(), ite = m_vVariableGroup.end(); it != ite; ++it, ++i)
    {
#if 0
        for (typename std::vector<variable_type>::const_iterator it2 = it->begin(); it2 != it->end(); ++it2)
        {
            if (m_model->variableName(*it2) == "v151841_1986" || m_model->variableName(*it2) == "v151841_1958" || m_model->variableName(*it2) == "v152237_1986" || m_model->variableName(*it2) == "v152237_1958")
                limboPrint(kDEBUG, "group %u, %s = %g\n", std::distance(m_vVariableGroup.begin(), it), m_model->variableName(*it2).c_str(), (double)m_vObjCoef[it2->id()]);
        }
#endif
        // skip fixed groups 
        if (m_vFixedGroup[i])
            continue; 
        // reset variables in this group 
        for (typename std::vector<variable_type>::const_iterator itt = it->begin(), itte = it->end(); itt != itte; ++itt)
            m_model->setVariableSolution(*itt, 0); 
        // find the bin with minimum cost for each item 
        typename std::vector<variable_type>::iterator itv = std::min_element(it->begin(), it->end(), CompareVariableByCoefficient(m_vObjCoef));
        m_model->setVariableSolution(*itv, 1);
    }
    // evaluate current objective 
    m_lagObj = evaluateLagObjective();

}
template <typename T, typename V>
void MultiKnapsackLagRelax<T, V>::computeSlackness() 
{
    // for each capacity constraint of bin 
    for (unsigned int i = 0, ie = m_vSlackness.size(); i < ie; ++i)
    {
        constraint_type const& constr = m_model->constraints().at(m_vConstraintPartition[i]); 
        // compute slackness with current solution 
        m_vSlackness[i] = m_model->evaluateConstraint(constr);
#if 0
        if (m_model->constraintName(constr) == "br1986" || m_model->constraintName(constr) == "br1958")
            limboPrint(kDEBUG, "bin %s slackness %g\n", m_model->constraintName(constr).c_str(), (double)m_vSlackness[i]);
#endif
    }
}
template <typename T, typename V>
typename MultiKnapsackLagRelax<T, V>::coefficient_value_type MultiKnapsackLagRelax<T, V>::evaluateLagObjective() const 
{
    // evaluate current objective 
    coefficient_value_type objValue = m_objConstant; 
    unsigned int i = 0; 
    for (typename std::vector<coefficient_value_type>::const_iterator it = m_vObjCoef.begin(); it != m_vObjCoef.end(); ++it, ++i)
        objValue += (*it)*m_model->variableSolution(variable_type(i));
    return objValue; 
}
template <typename T, typename V>
SolverProperty MultiKnapsackLagRelax<T, V>::converge()
{
    bool feasibleFlag = true; 
    bool convergeFlag = true; 
    for (unsigned int i = 0, ie = m_vLagMultiplier.size(); i < ie; ++i)
    {
        coefficient_value_type multiplier = m_vLagMultiplier[i];
        coefficient_value_type slackness = m_vSlackness[i];
        // must satisfy KKT condition 
        // lambda >= 0
        // g(x) <= 0
        // lambda * g(x) = 0
        //
        // g(x) = Ax-b = -slackness 
        if (slackness < 0)
        {
            convergeFlag = feasibleFlag = false; 
            break; 
        }
        else if (multiplier*slackness != 0)
        {
            convergeFlag = false; 
        }
    }
    SolverProperty status = INFEASIBLE; 
    // store feasible solutions with better objective 
    if (feasibleFlag)
    {
        coefficient_value_type obj = m_model->evaluateObjective(); 
        if (obj < m_bestObj)
        {
            m_vBestVariableSol = m_model->variableSolutions();
            m_bestObj = obj; 
        }
        status = SUBOPTIMAL;
    }
    if (convergeFlag)
        status = OPTIMAL;

    return status;
}
template <typename T, typename V>
SolverProperty MultiKnapsackLagRelax<T, V>::postProcess(typename MultiKnapsackLagRelax<T, V>::updater_type* updater, typename MultiKnapsackLagRelax<T, V>::searcher_type* searcher, SolverProperty status)
{
    if (status == OPTIMAL) // already OPTIMAL
        return status; 
    else if (m_bestObj != std::numeric_limits<coefficient_value_type>::max()) // there is a best feasible solution in store 
    {
        m_model->variableSolutions() = m_vBestVariableSol; 
        return SUBOPTIMAL;
    }
    else // no best solutions in store 
    {
        // try to search feasible solution with heuristic  
        return searcher->operator()(updater); 
    }
}
template <typename T, typename V>
bool MultiKnapsackLagRelax<T, V>::printVariableGroup(std::string const& filename) const 
{
    std::ofstream out (filename.c_str()); 
    if (!out.good())
    {
        limbo::limboPrint(kWARN, "failed to open %s for write", filename.c_str()); 
        return false; 
    }
    printVariableGroup(out);
    out.close();
    return true;
}
template <typename T, typename V>
std::ostream& MultiKnapsackLagRelax<T, V>::printVariableGroup(std::ostream& os) const 
{
    os << __func__ << " iteration " << m_iter << "\n";
    unsigned int i = 0; 
    for (typename std::vector<std::vector<variable_type> >::const_iterator it = m_vVariableGroup.begin(); it != m_vVariableGroup.end(); ++it, ++i)
    {
        os << "[" << i << "]";
        for (typename std::vector<variable_type>::const_iterator itv = it->begin(); itv != it->end(); ++itv)
        {
            if (m_model->variableSolution(*itv) == 1)
                os << " *" << m_model->variableName(*itv) << "*";
            else 
                os << " " << m_model->variableName(*itv);
        }
        os << "\n";
    }
    return os; 
}
template <typename T, typename V>
bool MultiKnapsackLagRelax<T, V>::printObjCoef(std::string const& filename) const 
{
    std::ofstream out (filename.c_str()); 
    if (!out.good())
    {
        limbo::limboPrint(kWARN, "failed to open %s for write", filename.c_str()); 
        return false; 
    }
    printObjCoef(out);
    out.close();
    return true;
}
template <typename T, typename V>
std::ostream& MultiKnapsackLagRelax<T, V>::printObjCoef(std::ostream& os) const 
{
    os << __func__ << " iteration " << m_iter << "\n";
    os << "lagrangian objective = " << m_lagObj << "\n";
    os << "objective = " << m_model->evaluateObjective() << "\n";
    for (unsigned int i = 0, ie = m_vObjCoef.size(); i < ie; ++i)
        os << m_model->variableName(variable_type(i)) << " = " << m_vObjCoef[i] << "\n";
    return os; 
}
template <typename T, typename V>
bool MultiKnapsackLagRelax<T, V>::printLagMultiplier(std::string const& filename) const 
{
    std::ofstream out (filename.c_str()); 
    if (!out.good())
    {
        limbo::limboPrint(kWARN, "failed to open %s for write", filename.c_str()); 
        return false; 
    }
    printLagMultiplier(out);
    out.close();
    return true;
}
template <typename T, typename V>
std::ostream& MultiKnapsackLagRelax<T, V>::printLagMultiplier(std::ostream& os) const 
{
    os << __func__ << " iteration " << m_iter << "\n";
    for (unsigned int i = 0, ie = m_vLagMultiplier.size(); i < ie; ++i)
        os << "[C" << m_vConstraintPartition[i] << "] = " << m_vLagMultiplier[i] 
            << " slack = " << m_vSlackness[i]
            << "\n";
    return os; 
}

/// @brief A base helper function object to update lagrangian multipliers using subgradient descent. 
/// All other schemes can be derived from this class 
/// @tparam T coefficient value type 
template <typename T>
class LagMultiplierUpdater
{
    public:
        /// @brief value type 
        typedef T value_type; 

        /// @brief constructor 
        LagMultiplierUpdater() {}
        /// @brief destructor 
        virtual ~LagMultiplierUpdater() {}

        /// @brief API to update lagrangian multiplier 
        /// @param iter current iteration 
        /// @param multiplier current multiplier value 
        /// @param slackness current slackness value assuming the constraint is in \f$ Ax \le b \f$ and compute \f$ b-Ax \f$
        /// @return updated multiplier value 
        virtual value_type operator()(unsigned int iter, value_type multiplier, value_type slackness) = 0;
};

/// @brief Update lagrangian multiplier with subgradient descent 
/// @tparam T coefficient value type 
template <typename T>
class SubGradientDescent : public LagMultiplierUpdater<T>
{
    public:
        /// @brief base type 
        typedef LagMultiplierUpdater<T> base_type;
        /// @brief value type 
        typedef typename base_type::value_type value_type;

        /// @brief constructor 
        /// @param alpha the power term for scaling factor \f$ t_k = \beta \cdot k^{-\alpha} \f$
        SubGradientDescent(value_type alpha = 1, value_type beta = 1)
            : SubGradientDescent::base_type()
            , m_alpha(alpha)
            , m_beta(beta)
            , m_iter(std::numeric_limits<unsigned int>::max())
            , m_scalingFactor(1)
        {
        }
        /// @brief copy constructor 
        /// @brief right hand side 
        SubGradientDescent(SubGradientDescent const& rhs)
        {
            copy(rhs);
        }
        /// @brief assignment 
        /// @brief right hand side 
        SubGradientDescent& operator=(SubGradientDescent const& rhs)
        {
            if (this != &rhs)
            {
                this->base_type::operator=(rhs);
                copy(rhs);
            }
            return *this; 
        }
        /// @brief destructor 
        ~SubGradientDescent()
        {
        }

        /// @brief API to update lagrangian multiplier using subgradient descent 
        /// @param iter current iteration 
        /// @param multiplier current multiplier value 
        /// @param slackness current slackness value assuming the constraint is in \f$ Ax \le b \f$ and compute \f$ b-Ax \f$
        /// @return updated multiplier value 
        value_type operator()(unsigned int iter, value_type multiplier, value_type slackness)
        {
            // avoid frequent computation of scaling factor  
            if (m_iter != iter)
            {
                m_iter = iter; 
                m_scalingFactor = pow((value_type)m_iter+1, -m_alpha)*m_beta;
            }
            value_type result = std::max((value_type)0, multiplier+m_scalingFactor*(-slackness));
            return result; 
        }
    protected:
        /// @brief copy object 
        /// @brief right hand side 
        void copy(SubGradientDescent const& rhs)
        {
            m_alpha = rhs.m_alpha;
            m_beta = rhs.m_beta;
            m_iter = rhs.m_iter; 
            m_scalingFactor = rhs.m_scalingFactor; 
        }

        value_type m_alpha; ///< power 
        value_type m_beta; ///< constant 
        unsigned int m_iter; ///< current iteration 
        value_type m_scalingFactor; ///< scaling factor \f$ t_k = \beta \cdot k^{-\alpha} \f$
};

/// @brief Base class for scaling scheme with default no scaling  
/// @tparam T coefficient value type 
/// @tparam V variable value type 
template <typename T, typename V>
class ProblemScaler
{
    public:
        /// @brief model type 
        typedef LinearModel<T, V> model_type; 
        /// @brief value type 
        typedef typename model_type::coefficient_value_type value_type; 
        /// @brief expression type 
        typedef typename model_type::expression_type expression_type; 
        /// @brief constraint type 
        typedef typename model_type::constraint_type constraint_type; 
        /// @brief term type 
        typedef typename model_type::term_type term_type; 

        /// @brief constructor 
        ProblemScaler() {}
        /// @brief destructor 
        virtual ~ProblemScaler() {}

        /// @brief API to compute scaling factor for expression using L2 norm  
        /// @param expr expression 
        /// @return scaling factor 
        virtual value_type operator()(expression_type const& /*expr*/) const
        {
            return 1; 
        }
        /// @brief API to compute scaling factor for constraints  using L2 norm 
        /// @param constr constraint 
        /// @return scaling factor 
        virtual value_type operator()(constraint_type const& constr) const
        {
            return this->operator()(constr.expression());
        }
};

/// @brief Scaling scheme with minimum coefficient in an expression 
/// @tparam T coefficient value type 
/// @tparam V variable value type 
template <typename T, typename V>
class MinCoefficientScaler : public ProblemScaler<T, V>
{
    public:
        /// @brief base type 
        typedef ProblemScaler<T, V> base_type;
        /// @brief model type 
        typedef typename base_type::model_type model_type; 
        /// @brief value type 
        typedef typename base_type::value_type value_type;
        /// @brief expression type 
        typedef typename base_type::expression_type expression_type; 
        /// @brief constraint type 
        typedef typename base_type::constraint_type constraint_type; 
        /// @brief term type 
        typedef typename base_type::term_type term_type; 

        /// @brief constructor 
        /// @param factor a constant scaling factor 
        MinCoefficientScaler(value_type factor = 1) : base_type(), m_scalingFactor(factor) {}
        /// @brief destructor 
        ~MinCoefficientScaler() {}

        /// @brief API to compute scaling factor for expression using L2 norm  
        /// @param expr expression 
        /// @return scaling factor 
        value_type operator()(expression_type const& expr) const
        {
            value_type result = std::numeric_limits<value_type>::max(); 
            for (typename std::vector<term_type>::const_iterator it = expr.terms().begin(), ite = expr.terms().end(); it != ite; ++it)
                result = std::min(result, it->coefficient()); 
            return result*m_scalingFactor;
        }
        /// @brief API to compute scaling factor for constraints  using L2 norm 
        /// @param constr constraint 
        /// @return scaling factor 
        value_type operator()(constraint_type const& constr) const
        {
            return this->operator()(constr.expression());
        }

    protected:
        value_type m_scalingFactor; ///< constant scaling factor 
};

/// @brief Scaling scheme with default L2 norm scaling  
/// @tparam T coefficient value type 
/// @tparam V variable value type 
template <typename T, typename V>
class L2NormScaler : public ProblemScaler<T, V>
{
    public:
        /// @brief base type 
        typedef ProblemScaler<T, V> base_type;
        /// @brief model type 
        typedef typename base_type::model_type model_type; 
        /// @brief value type 
        typedef typename base_type::value_type value_type;
        /// @brief expression type 
        typedef typename base_type::expression_type expression_type; 
        /// @brief constraint type 
        typedef typename base_type::constraint_type constraint_type; 
        /// @brief term type 
        typedef typename base_type::term_type term_type; 

        /// @brief constructor 
        L2NormScaler() : base_type() {}
        /// @brief destructor 
        ~L2NormScaler() {}

        /// @brief API to compute scaling factor for expression using L2 norm  
        /// @param expr expression 
        /// @return scaling factor 
        value_type operator()(expression_type const& expr) const
        {
            value_type result = 0; 
            for (typename std::vector<term_type>::const_iterator it = expr.terms().begin(), ite = expr.terms().end(); it != ite; ++it)
                result += it->coefficient()*it->coefficient(); 
            return sqrt(result/expr.terms().size()); 
        }
        /// @brief API to compute scaling factor for constraints  using L2 norm 
        /// @param constr constraint 
        /// @return scaling factor 
        value_type operator()(constraint_type const& constr) const
        {
            return this->operator()(constr.expression());
        }
};

/// @brief Base heuristic to search for feasible solutions 
/// @tparam T coefficient value type 
/// @tparam V variable value type 
template <typename T, typename V>
class FeasibleSearcher 
{
    public:
        /// @brief model type 
        typedef LinearModel<T, V> model_type; 
        /// @brief solver type 
        typedef MultiKnapsackLagRelax<T, V> solver_type; 
        /// @brief updater type for lagrangian multipliers 
        typedef typename solver_type::updater_type updater_type;
        /// @brief coefficient value type 
        typedef typename model_type::coefficient_value_type coefficient_value_type; 
        /// @brief variable value type 
        typedef typename model_type::variable_value_type variable_value_type; 
        /// @brief variable type 
        typedef typename model_type::variable_type variable_type; 
        /// @brief expression type 
        typedef typename model_type::expression_type expression_type; 
        /// @brief constraint type 
        typedef typename model_type::constraint_type constraint_type; 
        /// @brief term type 
        typedef typename model_type::term_type term_type; 

        /// @brief constructor 
        /// @param solver problem solver 
        FeasibleSearcher(solver_type* solver) 
            : m_solver(solver)
            , m_model(solver->m_model)
            , m_vObjCoef(solver->m_vObjCoef)
            , m_vVariableGroup(solver->m_vVariableGroup)
            , m_vConstraintPartition(solver->m_vConstraintPartition)
            , m_vLagMultiplier(solver->m_vLagMultiplier)
            , m_vSlackness(solver->m_vSlackness)
            , m_vScalingFactor(solver->m_vScalingFactor)
            , m_vFixedGroup(solver->m_vFixedGroup)
            , m_objConstant(solver->m_objConstant)
            , m_lagObj(solver->m_lagObj)
            , m_iter(solver->m_iter)
            , m_maxIters(solver->m_maxIters)
            , m_useInitialSol(solver->m_useInitialSol)
            , m_vBestVariableSol(solver->m_vBestVariableSol)
            , m_bestObj(solver->m_bestObj)
        {
        }
        /// @brief destructor 
        virtual ~FeasibleSearcher() {}

        /// @brief API to search for feasible solutions 
        /// @param updater updater for lagrangian multipliers 
        /// @return solving status 
        virtual SolverProperty operator()(updater_type* /*updater*/) {return INFEASIBLE;} 

    protected:
        solver_type* m_solver; ///< problem solver 
        model_type* m_model; ///< model for the problem 

        std::vector<coefficient_value_type>& m_vObjCoef; ///< coefficients variables in objective 
        std::vector<std::vector<variable_type> >& m_vVariableGroup; ///< group variables according to item 
        std::vector<unsigned int>& m_vConstraintPartition; ///< indices of constraints, the first partition is capacity constraints 
        std::vector<coefficient_value_type>& m_vLagMultiplier; ///< array of lagrangian multipliers 
        std::vector<coefficient_value_type>& m_vSlackness; ///< array of slackness values in each iteration, \f$ b-Ax \f$
        std::vector<coefficient_value_type>& m_vScalingFactor; ///< scaling factor for constraints and objective, last entry is for objective
        std::vector<bool>& m_vFixedGroup; ///< groups/items that are fixed, which means we will not change the assignment 
        coefficient_value_type& m_objConstant; ///< constant value in objective from lagrangian relaxation
        coefficient_value_type& m_lagObj; ///< current objective of the lagrangian subproblem 
        unsigned int& m_iter; ///< current iteration 
        unsigned int& m_maxIters; ///< maximum number of iterations 
        bool& m_useInitialSol; ///< whether use initial solutions or not 

        std::vector<variable_value_type>& m_vBestVariableSol; ///< best feasible solution found so far 
        coefficient_value_type& m_bestObj; ///< best objective found so far 
};

/// @brief Heuristic to search for feasible solutions by adjusting coefficients 
/// so that some items will not be assigned to some bins. 
/// @tparam T coefficient value type 
/// @tparam V variable value type 
template <typename T, typename V>
class SearchByAdjustCoefficient : public FeasibleSearcher<T, V>
{
    public:
        /// @brief base type 
        typedef FeasibleSearcher<T, V> base_type; 
        /// @brief model type 
        typedef typename base_type::model_type model_type; 
        /// @brief solver type 
        typedef typename base_type::solver_type solver_type; 
        /// @brief updater type for lagrangian multipliers 
        typedef typename solver_type::updater_type updater_type;
        /// @brief coefficient value type 
        typedef typename model_type::coefficient_value_type coefficient_value_type; 
        /// @brief variable value type 
        typedef typename model_type::variable_value_type variable_value_type; 
        /// @brief variable type 
        typedef typename model_type::variable_type variable_type; 
        /// @brief expression type 
        typedef typename model_type::expression_type expression_type; 
        /// @brief constraint type 
        typedef typename model_type::constraint_type constraint_type; 
        /// @brief term type 
        typedef typename model_type::term_type term_type; 

        /// @brief Wrapper for the move cost of an item 
        struct VariableMoveCost 
        {
            variable_type variable; ///< variable of the item 
            coefficient_value_type moveCost; ///< move cost 
            coefficient_value_type capacity; ///< capacity of the item 

            /// @brief constructor 
            /// @param var variable 
            /// @param mc move cost 
            /// @param cap capacity 
            VariableMoveCost(variable_type var, coefficient_value_type mc, coefficient_value_type cap)
                : variable(var)
                , moveCost(mc)
                , capacity(cap)
            {
            }
        };

        /// @brief Compare variables by its move cost 
        struct CompareVariableMoveCost
        {
            /// @param c1 pair of variable and cost 
            /// @param c2 pair of variable and cost 
            /// @return true if \a c1 has smaller cost ratio than \a c2
            bool operator()(VariableMoveCost const& c1, VariableMoveCost const& c2) const 
            {
                return c1.moveCost/(1+c1.capacity) < c2.moveCost/(1+c2.capacity); 
            }
        };

        /// @brief constructor 
        /// @param solver problem solver 
        SearchByAdjustCoefficient(solver_type* solver) : base_type(solver) {}
        /// @brief destructor 
        ~SearchByAdjustCoefficient() {}

        /// @brief API to search for feasible solutions 
        /// @param updater updater for lagrangian multipliers 
        /// @return solving status 
        virtual SolverProperty operator()(updater_type* updater)
        {
            SolverProperty status = INFEASIBLE; 
            // map variable to group 
            this->mapVariable2GroupAndConstr(); 
            // record number of bins with negative slack 
            unsigned int numNegativeSlacksPrev = std::numeric_limits<unsigned int>::max();
            unsigned int numNegativeSlacks = std::numeric_limits<unsigned int>::max();
            // mark whether a variable has been processed 
            std::vector<bool> vVariableProcess (this->m_model->numVariables(), false);
            // record move cost of items 
            std::vector<VariableMoveCost> vVariableMoveCost;
            // large cost 
            coefficient_value_type largeCost = 0; 
            for (typename std::vector<coefficient_value_type>::const_iterator it = this->m_vObjCoef.begin(), ite = this->m_vObjCoef.end(); it != ite; ++it)
                largeCost += (*it > 0)? *it : 0; 
            do 
            {
                vVariableProcess.assign(this->m_model->numVariables(), false);
                // update number of bins with negative slack for previous iteration 
                numNegativeSlacksPrev = numNegativeSlacks; 
                numNegativeSlacks = 0; 
                // go through all bins with negative slack 
                for (unsigned int i = 0, ie = this->m_vSlackness.size(); i < ie; ++i)
                {
                    coefficient_value_type slackness = this->m_vSlackness[i];
#if 0
                    if (this->m_model->constraintName(this->m_model->constraints()[i]) == "br1986" || this->m_model->constraintName(this->m_model->constraints()[i]) == "br1958")
                        limboPrint(kDEBUG, "slackness[%s] = %g\n", this->m_model->constraintName(this->m_model->constraints()[i]).c_str(), (double)slackness);
#endif
                    if (slackness < 0)
                    {
                        constraint_type const& constr = this->m_model->constraints()[i];
                        // compute move cost for items in this bin 
                        computeMoveCost(constr, vVariableProcess, vVariableMoveCost);  
                        // try to forbid some items to be assigned to this bin 
                        // sort items by its cost of moving to other bins 
                        std::sort(vVariableMoveCost.begin(), vVariableMoveCost.end(), CompareVariableMoveCost());

                        // the total capacity of items moving out 
                        for (typename std::vector<VariableMoveCost>::const_iterator it = vVariableMoveCost.begin(), ite = vVariableMoveCost.end(); it != ite; ++it)
                        {
                            if (slackness < 0)
                            {
                                // increase the cost of assigning the item to this bin 
                                //this->m_vObjCoef[it->variable.id()] += (it->moveCost+1)*100; 
                                this->m_vObjCoef[it->variable.id()] = largeCost; 
#if 0
                                if (this->m_model->constraintName(constr) == "br1986" || this->m_model->constraintName(constr) == "br1958")
                                    limboPrint(kDEBUG, "kick out %s %g\n", this->m_model->variableName(it->variable).c_str(), (double)this->m_vObjCoef[it->variable.id()]); 
#endif
                                // update total capacity of items moving out 
                                slackness += it->capacity; 
                                // mark as processed 
                                vVariableProcess[it->variable.id()] = true; 
                            }
                            else break; 
                        }
                        // compute number of bins with negative slack 
                        ++numNegativeSlacks; 
                    }
                }
                // mark fixed groups 
                //for (unsigned int i = 0, ie = this->m_vVariableGroup.size(); i < ie; ++i)
                //{
                //    for (typename std::vector<variable_type>::const_iterator it = this->m_vVariableGroup[i].begin(), ite = this->m_vVariableGroup[i].end(); it != ite; ++it)
                //    {
                //        // any variable not processed yet means that it is fixed 
                //        if (!vVariableProcess[it->id()] && this->m_model->variableSolution(*it)) 
                //        {
                //            this->m_vFixedGroup[i] = true; 
                //            break; 
                //        }
                //    }
                //}
#if 1
                limboPrint(kDEBUG, "numNegativeSlacks = %u\n", numNegativeSlacks); 
#endif
                if (numNegativeSlacks == 0)
                    break; 
                // run more iterations with the updated coefficients 
                status = this->m_solver->solveSubproblems(updater, this->m_iter+1, this->m_iter+this->m_maxIters/2);
            } while (numNegativeSlacks && numNegativeSlacks < numNegativeSlacksPrev);

            return status; 
        }
    protected:
        /// @brief construct mapping from variables to groups 
        void mapVariable2GroupAndConstr()
        {
            this->m_vVariable2Group.assign(this->m_model->numVariables(), std::numeric_limits<unsigned int>::max());
            for (unsigned int i = 0, ie = this->m_vVariableGroup.size(); i < ie; ++i)
                for (typename std::vector<variable_type>::const_iterator it = this->m_vVariableGroup[i].begin(), ite = this->m_vVariableGroup[i].end(); it != ite; ++it)
                    this->m_vVariable2Group[it->id()] = i; 
        }
        /// @brief compute move cost for an item to move out from current bin 
        /// @param constr constraint or bin 
        /// @param vVariableProcess flags denoting whether variables are processed or not 
        /// @param vVariableMoveCost array of move cost 
        void computeMoveCost(constraint_type const& constr, std::vector<bool> const& vVariableProcess, std::vector<VariableMoveCost>& vVariableMoveCost) const 
        {
            // extract items that are currently assigned to this bin 
            vVariableMoveCost.clear();
            unsigned int j = 0; 
            for (typename std::vector<term_type>::const_iterator it = constr.expression().terms().begin(), ite = constr.expression().terms().end(); it != ite; ++it, ++j)
            {
                // selected but not processed yet 
                if (this->m_model->variableSolution(it->variable()) && !vVariableProcess[it->variable().id()]) 
                {
                    // compute move cost of moving item to other bins 
                    coefficient_value_type moveCost = std::numeric_limits<coefficient_value_type>::max(); 
                    unsigned int group = this->m_vVariable2Group[it->variable().id()];
                    for (typename std::vector<variable_type>::const_iterator itv = this->m_vVariableGroup[group].begin(), itve = this->m_vVariableGroup[group].end(); itv != itve; ++itv)
                    {
                        if (it->variable() != *itv) // different variables 
                        {
                            //limboAssert(this->m_vObjCoef[itv->id()]-this->m_vObjCoef[it->variable().id()] >= 0); 
                            moveCost = std::min(moveCost, this->m_vObjCoef[itv->id()]-this->m_vObjCoef[it->variable().id()]);
                        }
                    }
                    // use the ratio of move cost to capacity 
                    vVariableMoveCost.push_back(VariableMoveCost(it->variable(), moveCost, it->coefficient()));
                }
            }
        }

        std::vector<unsigned int> m_vVariable2Group; ///< map variables to groups 
};

#if 0

/// @brief Heuristic to search for feasible solutions by select items for bins. 
/// @tparam T coefficient value type 
/// @tparam V variable value type 
template <typename T, typename V>
class SearchByBinSelection : public SearchByAdjustCoefficient<T, V>
{
    public:
        /// @brief base type 
        typedef SearchByAdjustCoefficient<T, V> base_type; 
        /// @brief model type 
        typedef typename base_type::model_type model_type; 
        /// @brief solver type 
        typedef typename base_type::solver_type solver_type; 
        /// @brief updater type for lagrangian multipliers 
        typedef typename solver_type::updater_type updater_type;
        /// @brief coefficient value type 
        typedef typename model_type::coefficient_value_type coefficient_value_type; 
        /// @brief variable type 
        typedef typename model_type::variable_type variable_type; 
        /// @brief expression type 
        typedef typename model_type::expression_type expression_type; 
        /// @brief constraint type 
        typedef typename model_type::constraint_type constraint_type; 
        /// @brief term type 
        typedef typename model_type::term_type term_type; 

        /// @brief compare pair by second element 
        struct ComparePairBySecond
        {
            /// @tparam PairType type of pair 
            /// @param p1 pair 
            /// @param p2 pair 
            /// @return true if \a p1 is smaller than \a p2 
            template <typename PairType>
            bool operator()(PairType const& p1, PairType const& p2) const 
            {
                return p1.second < p2.second; 
            }
        };

        /// @brief constructor 
        /// @param solver problem solver 
        SearchByBinSelection(solver_type* solver) : base_type(solver) {}
        /// @brief destructor 
        ~SearchByBinSelection() {}

        /// @brief API to search for feasible solutions 
        /// @param updater updater for lagrangian multipliers 
        /// @return solving status 
        virtual SolverProperty operator()(updater_type* /*updater*/) 
        {
            SolverProperty status = INFEASIBLE; 
            // map variable to group 
            this->mapVariable2Group();
            // map variable to constraint 
            this->mapVariable2Constraint(); 
            // mark whether a variable has been processed 
            std::vector<bool> vVariableProcess (this->m_model->numVariables(), false);
            // record whether an item has been processed 
            // we do not allow an item to be processed more than once 
            std::vector<bool> vGroupProcess (this->m_vVariableGroup.size(), false); 
            // record move cost of items 
            std::vector<typename base_type::VariableMoveCost> vVariableMoveCost;
            // collect remaining items, pair of group and capacity 
            // it does not work for items that have different capacities in different bins 
            std::vector<std::pair<unsigned int, coefficient_value_type> > vRemainGroup; 

            // go through all bins with negative slack 
            for (unsigned int i = 0, ie = this->m_vSlackness.size(); i < ie; ++i)
            {
                coefficient_value_type& slackness = this->m_vSlackness[i];
                constraint_type const& constr = this->m_model->constraints()[i];
                if (slackness < 0)
                {
                    // compute move cost for items in this bin 
                    this->computeMoveCost(constr, vVariableProcess, vVariableMoveCost);  
                    // try to forbid some items to be assigned to this bin 
                    // sort items by its cost of moving to other bins 
                    std::sort(vVariableMoveCost.begin(), vVariableMoveCost.end(), typename base_type::CompareVariableMoveCost());

                    // the total capacity of items moving out 
                    coefficient_value_type outCapacity = 0; 
                    for (typename std::vector<typename base_type::VariableMoveCost>::const_iterator it = vVariableMoveCost.begin(), ite = vVariableMoveCost.end(); it != ite; ++it)
                    {
                        if (outCapacity+slackness < 0)
                        {
                            // increase the cost of assigning the item to this bin 
                            this->m_vObjCoef[it->variable.id()] = std::numeric_limits<coefficient_value_type>::max(); 
                            // update total capacity of items moving out 
                            outCapacity += it->capacity; 
                            // revert variable solutions 
                            this->m_model->setVariableSolution(it->variable, 0); 
                            // collect item 
                            vRemainGroup.push_back(std::make_pair(this->m_vVariable2Group[it->variable.id()], it->capacity));
                            // mark as processed 
                            vVariableProcess[it->variable.id()] = true; 
                        }
                        else break; 
                    }
                    // update slack 
                    slackness += outCapacity; 
                }
                else // for bins with non-negative slack, mark all items as processed 
                {
                    for (typename std::vector<term_type>::const_iterator it = constr.expression().terms().begin(), ite = constr.expression().terms().end(); it != ite; ++it)
                        vGroupProcess[this->m_vVariable2Group[it->variable().id()]] = true; 
                }
            }

            // process remaining items 
            // build map from item to bins, only entries not processed yet are valid  
            std::vector<std::vector<unsigned int> > mMapGroup2Constrs (this->m_vVariableGroup.size()); 
            for (unsigned int i = 0, ie = this->m_vSlackness.size(); i < ie; ++i)
            {
                constraint_type const& constr = this->m_model->constraints()[i];
                for (typename std::vector<term_type>::const_iterator it = constr.expression().terms().begin(), ite = constr.expression().terms().end(); it != ite; ++it)
                {
                    unsigned int group = this->m_vVariable2Group[it->variable().id()]; 
                    if (!vGroupProcess[group])
                        mMapGroup2Constrs[group].push_back(i); 
                }
            }
            std::sort(vRemainGroup.begin(), vRemainGroup.end(), ComparePairBySecond()); 
            for (typename std::vector<std::pair<unsigned int, coefficient_value_type> >::const_reverse_iterator it = vRemainGroup.rbegin(), ite = vRemainGroup.rend(); it != ite; ++it)
            {
                unsigned int group = it->first; 
                // variable corresponding to the best bin 
                variable_type bestVar; 
                unsigned int bestConstr = std::numeric_limits<unsigned int>::max(); 
                coefficient_value_type bestCapInConstr = std::numeric_limits<coefficient_value_type>::max(); 
                coefficient_value_type bestCost = std::numeric_limits<coefficient_value_type>::max(); 
                for (std::vector<unsigned int>::const_iterator itb = mMapGroup2Constrs[group].begin(), itbe = mMapGroup2Constrs[group].end(); itb != itbe; ++itb)
                {
                    constraint_type const& constr = this->m_model->constraints()[*itb];
                    coefficient_value_type slackness = this->m_vSlackness[*itb];
                    for (typename std::vector<term_type>::const_iterator itv = constr.expression().terms().begin(), itve = constr.expression().terms().end(); itv != itve; ++itv)
                    {
                        if (this->m_vVariable2Group[itv->variable().id()] == group)
                        {
                            if (slackness >= itv->coefficient()) // bin is large enough 
                            {
                                if (this->m_vObjCoef[itv->variable().id()] < bestCost)
                                {
                                    bestVar = itv->variable(); 
                                    bestConstr = *itb; 
                                    bestCapInConstr = itv->coefficient(); 
                                    bestCost = this->m_vObjCoef[itv->variable().id()];
                                }
                            }
                        }
                    }
                }
                if (bestCost != std::numeric_limits<coefficient_value_type>::max())
                {
                    this->m_model->setVariableSolution(bestVar, 1);
                    this->m_vSlackness[bestConstr] -= bestCapInConstr;
                    vGroupProcess[group] = true; 
                }
            }

            // count items not assigned yet 
            unsigned int numUnprocess = std::count(vGroupProcess.begin(), vGroupProcess.end(), false); 
            if (numUnprocess == 0)
                status = SUBOPTIMAL; 
            limboPrint(kDEBUG, "numUnprocess = %u\n", numUnprocess);

            return status; 
        }
    protected:
        /// @brief construct mapping from variables to constraints  
        void mapVariable2Constraint()
        {
            this->m_mVariable2Constr.resize(this->m_model->numVariables());
            for (unsigned int i = 0, ie = this->m_vSlackness.size(); i < ie; ++i)
            {
                constraint_type const& constr = this->m_model->constraints()[i];
                unsigned int j = 0; 
                for (typename std::vector<term_type>::const_iterator it = constr.expression().terms().begin(), ite = constr.expression().terms().end(); it != ite; ++it, ++j)
                    this->m_mVariable2Constr[it->variable().id()].push_back(std::make_pair(i, j)); 
            }

            m_vObjCoefOrig.resize(this->m_model->numVariables(), 0); 
            for (typename std::vector<term_type>::const_iterator it = this->m_model->objective().terms().begin(), ite = this->m_model->objective().terms().end(); it != ite; ++it)
                m_vObjCoefOrig[it->variable().id()] = it->coefficient();
        }
        /// @brief compute move cost for an item to move out from current bin 
        /// @param constr constraint or bin 
        /// @param vVariableProcess flags denoting whether variables are processed or not 
        /// @param vVariableMoveCost array of move cost 
        void computeMoveCost(constraint_type const& constr, std::vector<bool> const& vVariableProcess, std::vector<VariableMoveCost>& vVariableMoveCost) const 
        {
            // extract items that are currently assigned to this bin 
            vVariableMoveCost.clear();
            unsigned int j = 0; 
            for (typename std::vector<term_type>::const_iterator it = constr.expression().terms().begin(), ite = constr.expression().terms().end(); it != ite; ++it, ++j)
            {
                // selected but not processed yet 
                if (this->m_model->variableSolution(it->variable()) && !vVariableProcess[it->variable().id()]) 
                {
                    // compute move cost of moving item to other bins 
                    coefficient_value_type moveCost = std::numeric_limits<coefficient_value_type>::max(); 
                    unsigned int group = this->m_vVariable2Group[it->variable().id()];
                    for (typename std::vector<variable_type>::const_iterator itv = this->m_vVariableGroup[group].begin(), itve = this->m_vVariableGroup[group].end(); itv != itve; ++itv)
                    {
                        if (it->variable() != *itv) // different variables 
                        {
                            // find the minimum slackness of target bins 
                            coefficient_value_type targetSlackness = std::numeric_limits<coefficient_value_type>::max(); 
                            coefficient_value_type targetCoeff = std::numeric_limits<coefficient_value_type>::max(); 
                            for (std::vector<std::pair<unsigned int, unsigned int> >::const_iterator itc = this->m_mVariable2Constr[itv->id()].begin(), itce = this->m_mVariable2Constr[itv->id()].end(); itc != itce; ++itc)
                            {
                                if (targetSlackness > this->m_vSlackness[itc->first])
                                {
                                    targetSlackness = this->m_vSlackness[itc->first]; 
                                    targetCoeff = this->m_model->constraints()[itc->first].expression().terms()[itc->second].coefficient();
                                }
                            }
                            if (targetSlackness < targetCoeff)
                                continue; 
                            coefficient_value_type targetCost = this->m_vObjCoefOrig[itv->id()]*std::max((coefficient_value_type)1, (targetCoeff+std::numeric_limits<coefficient_value_type>::epsilon())/(targetSlackness+std::numeric_limits<coefficient_value_type>::epsilon()));
                            moveCost = std::min(moveCost, targetCost);
                        }
                    }
                    // use the ratio of move cost to capacity 
                    vVariableMoveCost.push_back(VariableMoveCost(it->variable(), moveCost, it->coefficient()));
                }
            }
        }

        std::vector<std::vector<std::pair<unsigned int, unsigned int> > > m_mVariable2Constr; ///< map variables to constraints by pair of (constraint index, term index), a variable may have multiple constraints 
        std::vector<coefficient_value_type> m_vObjCoefOrig; ///< original coefficient of variable in objective 
};

#endif

} // namespace solvers 
} // namespace limbo 

#endif
