#include <Core/Geometry/Volume.hpp>
#include <Tests.hpp>

#include <random>
#include <set>

namespace Ra {
namespace Testing {

template <typename VolumeType>
void run() {
    using ValueType = typename VolumeType::ValueType;
    using IndexType = Core::Vector3i;
    using Core::Vector3;

    static std::random_device rd;    // Will be used to obtain a seed for the random number engine
    static std::mt19937 gen( rd() ); // Standard mersenne_twister_engine seeded with rd()

    static const Vector3 bounds( Scalar( 10. ), Scalar( 10. ), Scalar( 20. ) );
    static const IndexType nbBins( 64, 64, 100 );

    static std::uniform_real_distribution<Scalar> dis( Scalar( 0 ), Scalar( 1 ) );

    struct Query {
        IndexType index;
        ValueType value;

        static Query Random() {
            Query q;
            q.index = ( nbBins.array().cast<Scalar>() * Vector3::Random().cwiseAbs().array() )
                          .cast<int>();
            q.value = dis( gen );
            return q;
        }
    };
    struct QueryCmp {
        bool operator()( const Query& lhs, const Query& rhs ) const {
            return lhs.index( 0 ) == rhs.index( 0 )
                       ? ( lhs.index( 1 ) == rhs.index( 1 ) ? lhs.index( 2 ) < rhs.index( 2 )
                                                            : lhs.index( 1 ) < rhs.index( 1 ) )
                       : lhs.index( 0 ) < rhs.index( 0 );
        }
    };

    // create input samples, ensure that the query samples are not in stored in the volume
    size_t nbSamples = 1000;
    std::set<Query, QueryCmp> samples;

    size_t tries = 0;
    while ( samples.size() < nbSamples && tries++ < 3 * nbSamples )
    {
        Query q = Query::Random();
        samples.insert( q );
    }

    nbSamples = samples.size();
    const size_t nbStoredSamples = size_t( Scalar( 0.9 ) * Scalar( nbSamples ) );
    // const size_t nbQuerySamples  = nbSamples - nbStoredSamples;

    // Check volume configuration
    VolumeType t;
    t.setSize( nbBins );
    RA_VERIFY( nbBins.isApprox( std::size( t ) ), "Invalid container size" );

    Vector3 binSize = bounds.cwiseQuotient( nbBins.cast<Scalar>() );
    t.setBinSize( binSize );

    // check insertion
    auto it = samples.begin();
    for ( size_t i = 0; i != nbStoredSamples; ++i, ++it )
    {
        RA_VERIFY( t.addToBin( it->value, it->index ), "Error when inserting valid element" );
    }

    it = samples.begin();
    // Check values have been correctly inserted
    for ( size_t i = 0; i != nbStoredSamples; ++i, ++it )
    {
        auto query = binSize.cwiseProduct( it->index.template cast<Scalar>() ).eval();
        RA_VERIFY( t.getValue( query ), "Cannot access stored value" );
        RA_VERIFY( Testing::isApprox( t.getValue( query ), it->value ),
                   "Inconsistent stored value" );
    }

    // check new values are not already in the set but will be correcly added
    for ( size_t i = nbStoredSamples; i != nbSamples; ++i, ++it )
    {
        auto query = binSize.cwiseProduct( it->index.template cast<Scalar>() ).eval();
        auto result = t.getValue( query ); // can be either invalid, or default value
        RA_VERIFY( t.addToBin( it->value, it->index ), "Error when inserting valid element" );
        RA_VERIFY( result != t.getValue( query ), "Value has not been correctly inserted" );
    }

    t.clear();
}
} // namespace Testing
} // namespace Ra

int main( int /*argc*/, const char** argv ) {
    using namespace Ra;
    using Core::Geometry::VolumeGrid;
    using Core::Geometry::VolumeSparse;

    if ( !Testing::init_testing( 1, argv ) )
    {
        return EXIT_FAILURE;
    }

#pragma omp parallel for
    for ( int i = 0; i < Testing::g_repeat; ++i )
    {
        CALL_SUBTEST( ( Testing::run<VolumeGrid>() ) );
        CALL_SUBTEST( ( Testing::run<VolumeSparse>() ) );
    }

    return EXIT_SUCCESS;
}
