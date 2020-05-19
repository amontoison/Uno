#ifndef FILTER_H
#define FILTER_H

#include <ostream>
#include <vector>
#include <list>
#include <map>
#include <memory>

struct FilterConstants {
    double Beta; /*!< Margin around filter */
    double Gamma; /*!< Margin around filter (sloping margin) */
};

struct FilterEntry {
    double infeasibility_measure;
    double optimality_measure;
};

/*! \class Filter
 * \brief Filter
 *
 *  Filter
 */
class Filter {
public:
    Filter(FilterConstants& constants);
    virtual ~Filter();

    double upper_bound; /*!< Upper bound on constraint violation */
    unsigned int max_size; /*!< Max filter size */
    FilterConstants constants; /*!< Set of constants */

    void reset();
    virtual void add(double infeasibility_measure, double optimality_measure);
    virtual bool accept(double infeasibility_measure, double optimality_measure);
    virtual bool improves_current_iterate(double current_infeasibility_measure, double current_optimality_measure, double trial_infeasibility_measure, double trial_optimality_measure);
    virtual double compute_actual_reduction(double current_objective, double current_residual, double trial_objective);

    double eta_min();
    double omega_min();

    friend std::ostream& operator<<(std::ostream &stream, Filter& filter);
    
protected:
    std::list<FilterEntry> entries;
};

/*! \class NonmonotoneFilter
 * \brief Non-monotonic filter
 *
 *  Non-monotonic filter
 */
class NonmonotoneFilter : public Filter {
public:
    NonmonotoneFilter(FilterConstants& constants, int number_dominated_entries = 3);

    void add(double infeasibility_measure, double optimality_measure) override;
    bool accept(double infeasibility_measure, double optimality_measure) override;
    bool improves_current_iterate(double current_infeasibility_measure, double current_optimality_measure, double trial_infeasibility_measure, double trial_optimality_measure) override;
    double compute_actual_reduction(double current_objective, double current_residual, double trial_objective) override;
    void shift_left(int start, int length);
    void shift_right(int start, int length);

    int number_dominated_entries; /*!< Memory of filter */
};

class FilterFactory {
public:
    static std::shared_ptr<Filter> create(std::map<std::string, std::string> options);
};

#endif // FILTER_H