#ifndef BOOST_BOOSTEST_H_
#define BOOST_BOOSTEST_H_

#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics/stats.hpp>
#include <boost/accumulators/statistics/mean.hpp>
#include <boost/accumulators/statistics/moment.hpp>

using namespace std;

class boostest
{
public:

    boostest()
    {
    }

    virtual ~boostest()
    {
    }

    int m_boostest()
    {
        using namespace boost::accumulators;
        // Define an accumulator set for calculating the mean and the
        // 2nd moment ...
        accumulator_set<double, stats<tag::mean, tag::moment<2> > > acc;

        // push in some data ...
        acc(1.2);
        acc(2.3);
        acc(3.4);
        acc(4.5);

        // Display the results ...
        std::cout << "Mean:   " << mean(acc) << std::endl;
        std::cout << "Moment: " << boost::accumulators::moment<2>(acc) << std::endl;

        return 0;
    }
};
#endif //BOOST_BOOSTEST_H_
