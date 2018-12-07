#include <Core/Animation/Skinning/LinearBlendSkinning.hpp>

namespace Ra {
namespace Core {
namespace Animation {

void linearBlendSkinning( const Vector3Array& inMesh, const Pose& pose, const WeightMatrix& weight,
                          Vector3Array& outMesh ) {
    outMesh.clear();
    outMesh.resize( inMesh.size(), Vector3::Zero() );
    for ( int k = 0; k < weight.outerSize(); ++k )
    {
        const int nonZero = weight.col( k ).nonZeros();
        WeightMatrix::InnerIterator it0( weight, k );
#pragma omp parallel for
        for ( int nz = 0; nz < nonZero; ++nz )
        {
            WeightMatrix::InnerIterator it = it0 + Eigen::Index( nz );
            const uint i = it.row();
            const uint j = it.col();
            const Scalar w = it.value();
            outMesh[i] += w * ( pose[j] * inMesh[i] );
        }
    }
}

void RA_CORE_API linearBlendSkinningSTBS( const Vector3Array& inMesh, const Pose& pose,
                                          const Skeleton& poseSkel, const Skeleton& restSkel,
                                          const WeightMatrix& weightLBS, const WeightMatrix& weightSTBS,
                                          Vector3Array& outMesh ) {
    outMesh.clear();
    outMesh.resize( inMesh.size(), Vector3::Zero() );
    // first decompose all pose transforms
    Ra::Core::VectorArray<Matrix3> R( pose.size() );
    Matrix3 S; // we don't mind it
#pragma omp parallel for
    for ( int i=0; i<pose.size(); ++i )
    {
        pose[i].computeRotationScaling( &R[i], &S );
    }
    // then go through vertices
    for ( int k = 0; k < weightLBS.outerSize(); ++k )
    {
        const int nonZero = weightLBS.col( k ).nonZeros();
        WeightMatrix::InnerIterator it0( weightLBS, k );
#pragma omp parallel for
        for ( int nz = 0; nz < nonZero; ++nz )
        {
            WeightMatrix::InnerIterator it = it0 + Eigen::Index( nz );
            const uint i = it.row();
            const uint j = it.col();
            const Scalar w = it.value();
            Vector3 a, b, a_, b_;
            restSkel.getBonePoints( j, a, b );
            poseSkel.getBonePoints( j, a_, b_ );
            Vector3 si = ( std::sqrt( ( b_ - a_ ).squaredNorm() / ( b - a ).squaredNorm() ) - 1) * ( b - a );
            outMesh[i] += w * ( a_ + R[j] * ( weightSTBS.coeff(i,j) * si - a + inMesh[i] ) );
        }
    }
}

} // namespace Animation
} // namespace Core
} // namespace Ra
