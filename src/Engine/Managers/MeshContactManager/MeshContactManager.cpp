﻿#include <Engine/Managers/MeshContactManager/MeshContactManager.hpp>

#include <string>
#include <iostream>

#include <Core/Tasks/TaskQueue.hpp>
#include <Core/File/FileData.hpp>
#include <Core/File/HandleData.hpp>

#include <Engine/RadiumEngine.hpp>

#include <Engine/Entity/Entity.hpp>

#include "Eigen/Core"

#include <Core/Geometry/Normal/Normal.hpp>

#include <Engine/Managers/SystemDisplay/SystemDisplay.hpp>

#include <iostream>
#include <fstream>

namespace Ra
{
    namespace Engine
    {

        MeshContactManager::MeshContactManager()
            :m_nb_faces_max( 0 )
            ,m_nbfacesinit( 0 )
            ,m_nbfaces( 0 )
            ,m_nbobjects( 0 )
            ,m_threshold( 0.0 )
            ,m_broader_threshold ( 0.0 )
            ,m_lambda( 0.0 )
            ,m_m( 2.0 )
            ,m_n ( 2.0 )
            ,m_influence ( 0.9 )
            ,m_asymmetry ( 0.0 )
            ,m_curr_vsplit( 0 )
            ,m_threshold_max( 0 )
            ,m_asymmetry_max( 0 )
        {
        }

        void MeshContactManager::setNbFacesChanged(const int nb)
        {
            m_nbfacesinit = nb;
        }

        void MeshContactManager::setNbObjectsChanged(const int nb)
        {
            m_nbobjects = nb;
        }

        void MeshContactManager::computeNbFacesMax()
        {
            m_nb_faces_max = 0;

            for (const auto& elem : m_meshContactElements)
            {
                m_nb_faces_max += static_cast<MeshContactElement*>(elem)->getNbFacesMax();
            }
        }

        void MeshContactManager::computeNbFacesMax2()
        {
            m_nb_faces_max = 0;

            for (uint i = 0; i < m_initTriangleMeshes.size(); i++)
            {
                m_nb_faces_max += m_initTriangleMeshes[i].m_triangles.size();
            }
        }

        void MeshContactManager::setThresholdChanged(const double threshold)
        {
            m_threshold = threshold;
        }

        void MeshContactManager::setLambdaChanged(const double lambda)
        {
            m_lambda = lambda;
        }

        void MeshContactManager::setMChanged(const double m)
        {
            m_m = m;
        }

        void MeshContactManager::setNChanged(const double n)
        {
            m_n = n;
        }

        void MeshContactManager::setInfluenceChanged(const double influence)
        {
            m_influence = influence;
        }

        void MeshContactManager::setAsymmetryChanged(const double asymmetry)
        {
            m_asymmetry = asymmetry;
        }

        void MeshContactManager::addMesh(MeshContactElement* mesh, const std::string& entityName, const std::string& componentName)
        {
            m_meshContactElements.push_back(mesh);

            //mesh->computeTriangleMesh();
            mesh->computeMesh(entityName,componentName);
            m_initTriangleMeshes.push_back(mesh->getInitTriangleMesh());

            Super4PCS::TriangleKdTree<>* trianglekdtree = new Super4PCS::TriangleKdTree<>();
            m_trianglekdtrees.push_back(trianglekdtree);
            m_trianglekdtrees[m_trianglekdtrees.size()-1] = mesh->computeTriangleKdTree(m_initTriangleMeshes[m_initTriangleMeshes.size()-1]);
            LOG(logINFO) << "m_trianglekdtrees size : " << m_trianglekdtrees.size();

            m_nbobjects++;
        }

        void MeshContactManager::computeThreshold()
        {
            m_thresholds.setZero();
            Scalar min_th = std::numeric_limits<Scalar>::max();
            Scalar min_th_obj;
            Scalar th;
            std::pair<Ra::Core::Index,Scalar> triangle;
            for (uint i = 0; i < m_meshContactElements.size(); i++)
            {
                MeshContactElement* obj1 = m_meshContactElements[i];
                Ra::Core::VectorArray<Ra::Core::Triangle> t1 = obj1->getInitTriangleMesh().m_triangles;
                Ra::Core::VectorArray<Ra::Core::Vector3> v1 = obj1->getInitTriangleMesh().m_vertices;
                for (uint j = i + 1; j < m_meshContactElements.size(); j++)
                {
                    min_th_obj = std::numeric_limits<Scalar>::max();
                    // for each face of an object, we find the closest face in the other object
                    for (uint k = 0; k < t1.size(); k++)
                    {
                        triangle = m_trianglekdtrees[j]->doQueryRestrictedClosestIndexTriangle(v1[t1[k][0]],v1[t1[k][1]],v1[t1[k][2]]);
                        CORE_ASSERT(triangle.first > -1, "Invalid triangle index.");
                        th = triangle.second;
                        if (th < min_th_obj)
                        {
                            min_th_obj = th;
                        }
                    }
                    m_thresholds(i,j) = min_th_obj;
                    if (min_th_obj < min_th)
                    {
                        min_th = min_th_obj;
                    }
                }
            }
            m_broader_threshold = min_th;
        }

        void MeshContactManager::computeThresholdTest()
        {
            Scalar min_th = std::numeric_limits<Scalar>::max();
            Scalar th;
            for (uint i = 0; i < m_meshContactElements.size(); i++)
            {
                MeshContactElement* obj1 = m_meshContactElements[i];
                Ra::Core::VectorArray<Ra::Core::Triangle> t1 = obj1->getInitTriangleMesh().m_triangles;
                Ra::Core::VectorArray<Ra::Core::Vector3> v1 = obj1->getInitTriangleMesh().m_vertices;
                for (uint j = i + 1; j < m_meshContactElements.size(); j++)
                {
                    MeshContactElement* obj2 = m_meshContactElements[j];
                    Ra::Core::VectorArray<Ra::Core::Triangle> t2 = obj2->getInitTriangleMesh().m_triangles;
                    Ra::Core::VectorArray<Ra::Core::Vector3> v2 = obj2->getInitTriangleMesh().m_vertices;
                    // for each face of an object, we find the closest face in the other object
                    for (uint k = 0; k < t1.size(); k++)
                    {
                        for (uint l = 0; l < t2.size(); l++)
                        {
                            const Ra::Core::Vector3 triangle1[3] = {v1[t1[k][0]], v1[t1[k][1]], v1[t1[k][2]]};
                            const Ra::Core::Vector3 triangle2[3] = {v2[t2[l][0]], v2[t2[l][1]], v2[t2[l][2]]};
                            th = Ra::Core::DistanceQueries::triangleToTriSq(triangle1,triangle2).distance;
                            if (th < min_th)
                            {
                                min_th = th;
                            }
                        }
                    }
                }
            }
            LOG(logINFO) << "Test threshold computed value : " << min_th;
        }

        void MeshContactManager::distanceDistribution()
        {
            Scalar dist;
            Scalar step;
            Scalar th;
            std::pair<Ra::Core::Index,Scalar> triangle;
            for (uint i = 0; i < m_meshContactElements.size(); i++)
            {
                MeshContactElement* obj1 = m_meshContactElements[i];
                Ra::Core::VectorArray<Ra::Core::Triangle> t1 = obj1->getInitTriangleMesh().m_triangles;
                Ra::Core::VectorArray<Ra::Core::Vector3> v1 = obj1->getInitTriangleMesh().m_vertices;
                Super4PCS::AABB3D aabb1 = Super4PCS::AABB3D();
                for (uint a = 0; a < v1.size(); a++)
                {
                    aabb1.extendTo(v1[a]);
                }
                for (uint j = 0; j < m_meshContactElements.size(); j++)
                {
                    if (j != i)
                    {
                        MeshContactElement* obj2 = m_meshContactElements[j];
                        Ra::Core::VectorArray<Ra::Core::Vector3> v2 = obj2->getInitTriangleMesh().m_vertices;
                        Super4PCS::AABB3D aabb2 = Super4PCS::AABB3D();
                        for (uint b = 0; b < v2.size(); b++)
                        {
                            aabb2.extendTo(v2[b]);
                        }
                        dist = (aabb1.center() - aabb2.center()).norm() / 2;
                        step = dist / NBMAX_STEP;
                        uint distArray[NBMAX_STEP] = {0};
                        uint first, last, mid;

                        // for each face of an object, we find the closest face in the other object
                        for (uint k = 0; k < t1.size(); k++)
                        {
                            triangle = m_trianglekdtrees[j]->doQueryRestrictedClosestIndexTriangle(v1[t1[k][0]],v1[t1[k][1]],v1[t1[k][2]]);
                            CORE_ASSERT(triangle.first > -1, "Invalid triangle index.");
                            th = triangle.second;

                            first = 0;
                            last = NBMAX_STEP;
                            mid = NBMAX_STEP / 2;
                            while ((first + 1) < last)
                            {
                                if (th <= mid * step)
                                {
                                    last = mid;
                                    mid = (first + last) / 2;
                                }
                                else
                                {
                                    first = mid;
                                    mid = (first + last) / 2;
                                }
                            }
                            if (last == NBMAX_STEP)
                            {
                                if (th <= NBMAX_STEP * step)
                                    distArray[first]++;
                            }
                            else
                            {
                                distArray[first]++;
                            }
                        }

                        std::ofstream file("Dist_" + std::to_string(i) + "_" + std::to_string(j) + ".txt", std::ios::out | std::ios::trunc);
                        CORE_ASSERT(file, "Error while opening distance distribution file.");
                        for (uint k = 0; k < NBMAX_STEP; k++)
                        {
                           file << step * (k + 1)<< " " << distArray[k] << std::endl;
                        }
                        file.close();
                    }
                }
            }
        }

        void MeshContactManager::compareDistanceDistribution()
        {
            Scalar step;
            int nb1, nb2;

            for (uint i = 0; i < m_meshContactElements.size(); i++)
            {
                for (uint j = i+1; j < m_meshContactElements.size(); j++)
                {
                        std::ifstream file1("Dist_" + std::to_string(i) + "_" + std::to_string(j) + ".txt", std::ios::in);
                        std::ifstream file2("Dist_" + std::to_string(j) + "_" + std::to_string(i) + ".txt", std::ios::in);
                        CORE_ASSERT(file1 && file2, "Error while opening distance distribution files.");

                        std::ofstream file3("Diff_" + std::to_string(i) + "_" + std::to_string(j) + ".txt", std::ios::out | std::ios::trunc);
                        CORE_ASSERT(file3, "Error while opening comparaison distance distribution file.");
                        while (!file1.eof())
                        {
                            file1 >> step >> nb1;
                            file2 >> step >> nb2;
                            file3 << step << " " << abs(nb2-nb1) << std::endl;
                        }
                        file1.close();
                        file2.close();
                        file3.close();
                }
            }
        }

        // Data to be loaded next time around
        void MeshContactManager::distanceAsymmetryDistribution()
        {
            std::ofstream file("Data.txt", std::ios::out | std::ios::trunc);
            CORE_ASSERT(file, "Error while opening distance asymmetry distribution file.");

            std::pair<Ra::Core::Index,Scalar> triangle;

            for (uint i = 0; i < m_meshContactElements.size(); i++)
            {
                MeshContactElement* obj1 = m_meshContactElements[i];
                Ra::Core::VectorArray<Ra::Core::Triangle> t1 = obj1->getInitTriangleMesh().m_triangles;
                Ra::Core::VectorArray<Ra::Core::Vector3> v1 = obj1->getInitTriangleMesh().m_vertices;
//                Super4PCS::AABB3D aabb1 = Super4PCS::AABB3D();
//                for (uint a = 0; a < v1.size(); a++)
//                {
//                    aabb1.extendTo(v1[a]);
//                }

                std::vector<std::vector<std::pair<Ra::Core::Index,Scalar> > > obj1_distances;

                for (uint j = 0; j < m_meshContactElements.size(); j++)
                {

                    std::vector<std::pair<Ra::Core::Index,Scalar> > distances;

                    if (j != i)
                    {
//                        MeshContactElement* obj2 = m_meshContactElements[j];
//                        Ra::Core::VectorArray<Ra::Core::Vector3> v2 = obj2->getInitTriangleMesh().m_vertices;
//                        Super4PCS::AABB3D aabb2 = Super4PCS::AABB3D();
//                        for (uint b = 0; b < v2.size(); b++)
//                        {
//                            aabb2.extendTo(v2[b]);
//                        }

//                        // intersection between the 2 bboxes to see if it's worth doing the following
//                        if (aabb1.intersection(aabb2))
//                        {

                            // for each face of an object, we find the closest face in the other object
                            for (uint k = 0; k < t1.size(); k++)
                            {
                                triangle = m_trianglekdtrees[j]->doQueryRestrictedClosestIndexTriangle(v1[t1[k][0]],v1[t1[k][1]],v1[t1[k][2]]);
                                CORE_ASSERT(triangle.first > -1, "Invalid triangle index.");

                                distances.push_back(triangle);

                                file << triangle.first << " " << triangle.second << std::endl;
                            }

//                        }
                    }

                    obj1_distances.push_back(distances);
                }
                m_distances.push_back(obj1_distances);
            }
            file.close();

            sortDistAsymm(); // to move in case there are more than 2 objects and it's not needed
        }

        void MeshContactManager::loadDistribution(std::string filePath)
        {
            std::ifstream file(filePath, std::ios::in);
            CORE_ASSERT(file, "Error while opening distance asymmetry distributions file.");

//            if (filePath != "")
//            {
                int id;
                Scalar dist;

                std::pair<Ra::Core::Index,Scalar> triangle;

                for (uint i = 0; i < m_meshContactElements.size(); i++)
                {
                    std::vector<std::vector<std::pair<Ra::Core::Index,Scalar> > > obj_distances;

                    int nbFaces = m_meshContactElements[i]->getInitTriangleMesh().m_triangles.size();

                    for (uint j = 0; j < m_meshContactElements.size(); j++)
                    {
                        std::vector<std::pair<Ra::Core::Index,Scalar> > distances;

                        if (j != i)
                        {
                            for (uint k = 0; k < nbFaces; k++)
                            {
                                file >> id >> dist;
                                triangle.first = id;
                                triangle.second = dist;
                                distances.push_back(triangle);
                            }

                        }

                        obj_distances.push_back(distances);
                    }

                    m_distances.push_back(obj_distances);
                }
//            }

                sortDistAsymm(); // to move in case there are more than 2 objects and it's not needed
        }

        void MeshContactManager::sortDistAsymm()
        {
            for (uint i = 0; i < m_meshContactElements.size(); i++)
            {
                int nbFaces = m_meshContactElements[i]->getInitTriangleMesh().m_triangles.size();

                for (uint j = 0; j < m_meshContactElements.size(); j++)
                {
                    if (j != i)
                    {
                        for (uint k = 0; k < nbFaces; k++)
                        {
                            PtDistrib pt;
                            pt.objId = i;
                            pt.faceId = k;
                            pt.r = m_distances[i][j][k].second;
                            pt.a = abs(m_distances[i][j][k].second - m_distances[j][i][m_distances[i][j][k].first].second);
                            m_distrib.push_back(pt);
                            if (pt.r > m_threshold_max)
                            {
                                m_threshold_max = pt.r;
                            }
                            if (pt.a > m_asymmetry_max)
                            {
                                m_asymmetry_max = pt.a;
                            }
                        }
                    }
                }
            }
        }

        // value that will be used for the slider in UI
        int MeshContactManager::getThresholdMax()
        {
            return ((int)(m_threshold_max * PRECISION) + 1);
        }

        // value that will be used for the slider in UI
        int MeshContactManager::getAsymmetryMax()
        {
           return ((int)(m_asymmetry_max * PRECISION) + 1);
        }

        // works well in case of 2 objects only
        void MeshContactManager::displayDistribution(Scalar distValue, Scalar asymmValue)
        {
            //Ra::Core::Vector4 color(0, 0, 1, 0);

            Ra::Core::Vector4 distColor(1, 0, 0, 0);
            Ra::Core::Vector4 asymmColor(0, 1, 0, 0);

            Ra::Core::Vector4 vertexColor (0, 0, 0, 0);
            for (uint i = 0; i < m_meshContactElements.size(); i++)
            {
                MeshContactElement* obj = m_meshContactElements[i];
                int nbVertices = obj->getMesh()->getGeometry().m_vertices.size();
                Ra::Core::Vector4Array colors;
                for (uint v = 0; v < nbVertices; v++)
                {
                    colors.push_back(vertexColor);
                }
                obj->getMesh()->addData(Ra::Engine::Mesh::VERTEX_COLOR, colors);
            }

            for (uint j = 0; j < m_distrib.size(); j++)
            {
                if (m_distrib[j].r <= distValue && m_distrib[j].a <= asymmValue)
                {
                    MeshContactElement* obj = m_meshContactElements[m_distrib[j].objId];
                    //Ra::Core::VectorArray<Ra::Core::Triangle> t = obj->getInitTriangleMesh().m_triangles;
                    Ra::Core::VectorArray<Ra::Core::Triangle> t = obj->getTriangleMeshDuplicate().m_triangles;
                    Ra::Core::Vector4Array colors = obj->getMesh()->getData(Ra::Engine::Mesh::VERTEX_COLOR);

                    Scalar distCoeff, asymmCoeff;
                    if (distValue != 0)
                    {
                        distCoeff = (distValue - m_distrib[j].r) / distValue;
                    }
                    else
                    {
                        distCoeff = 1;
                    }
                    if (asymmValue != 0)
                    {
                        asymmCoeff = (asymmValue - m_distrib[j].a) / asymmValue;
                    }
                    else
                    {
                        asymmCoeff = 1;
                    }

                    colors[t[m_distrib[j].faceId][0]] = distCoeff * distColor + asymmCoeff * asymmColor;
                    colors[t[m_distrib[j].faceId][1]] = distCoeff * distColor + asymmCoeff * asymmColor;
                    colors[t[m_distrib[j].faceId][2]] = distCoeff * distColor + asymmCoeff * asymmColor;
                    obj->getMesh()->addData(Ra::Engine::Mesh::VERTEX_COLOR, colors);
                }
            }
        }

        // Displaying colored faces in light of r and a
        void MeshContactManager::setDisplayDistribution()
        {
            //sortDistAsymm();

            // reloading initial mesh in case of successive simplifications
            for (uint objIndex = 0; objIndex < m_meshContactElements.size(); objIndex++)
            {
                m_meshContactElements[objIndex]->setMesh(m_meshContactElements[objIndex]->getTriangleMeshDuplicate());
            }

            displayDistribution(m_threshold, m_asymmetry);
        }

        void MeshContactManager::distanceAsymmetryFiles()
        {
            Scalar dist, asymm;

            // for each object
            for (uint i = 0; i < m_distances.size(); i++)
            {
                // for each other object whose bbox intersects the one of the object
                for (uint j = i + 1; j < m_distances[i].size(); j++)
                {
                    std::ofstream file("Distrib_" + std::to_string(i) + "_" + std::to_string(j) + ".txt", std::ios::out | std::ios::trunc);
                    CORE_ASSERT(file, "Error while opening distance distribution file.");

                    // for each face of the object the closest face in the other object is found
                    for (uint k = 0; k < m_distances[i][j].size(); k++)
                    {
                        std::pair<Ra::Core::Index,Scalar> triangle1 = m_distances[i][j][k];

                        std::pair<Ra::Core::Index,Scalar> triangle2 = m_distances[j][i][triangle1.first];
                        dist = (triangle1.second + triangle2.second) / 2;
                        asymm = abs(triangle1.second - triangle2.second);
                        file << dist << " " << asymm << std::endl;
                    }

                    // for each face of the other object the closest face in the object is found
                    for (uint k = 0; k < m_distances[j][i].size(); k++)
                    {
                        std::pair<Ra::Core::Index,Scalar> triangle1 = m_distances[j][i][k];

                        std::pair<Ra::Core::Index,Scalar> triangle2 = m_distances[i][j][triangle1.first];
                        dist = (triangle1.second + triangle2.second) / 2;
                        asymm = abs(triangle1.second - triangle2.second);
                        file << dist << " " << asymm << std::endl;
                    }

                    file.close();
                }
            }
        }

        void MeshContactManager::distanceAsymmetryFile()
        {
            Scalar dist, asymm;

            std::ofstream file("Distrib.txt", std::ios::out | std::ios::trunc);
            CORE_ASSERT(file, "Error while opening distance distribution file.");

            for (uint i = 0; i < m_distances.size(); i++)
            {
                for (uint j = 0; j < m_distances[i].size(); j++)
                {
                    for (uint k = 0; k < m_distances[i][j].size(); k++)
                    {
                        dist = m_distances[i][j][k].second;
                        asymm = abs(m_distances[i][j][k].second - m_distances[j][i][m_distances[i][j][k].first].second);
                        file << dist << " " << asymm << std::endl;
                    }
                }
            }

            file.close();
        }

        void MeshContactManager::thresholdComputation()
        {
            m_broader_threshold = m_threshold / (std::pow(1 - std::pow(m_influence,1/m_n),1/m_m));
        }

        void MeshContactManager::setLodValueChanged(int value)
        {
            if (m_nbfaces < value)
            {
                while (m_nbfaces < value)
                {
                    MeshContactElement* obj = static_cast<MeshContactElement*>(m_meshContactElements[m_index_pmdata[--m_curr_vsplit]]);
                    int nbfaces = obj->getProgressiveMeshLOD()->getProgressiveMesh()->getNbFaces();
                    if (! obj->getProgressiveMeshLOD()->more())
                        break;
                    else
                    {
                        m_nbfaces += (obj->getProgressiveMeshLOD()->getProgressiveMesh()->getNbFaces() - nbfaces);
                    }
                }
            }
            else if (m_nbfaces > value)
            {
                while (m_nbfaces > value)
                {
                    MeshContactElement* obj = static_cast<MeshContactElement*>(m_meshContactElements[m_index_pmdata[m_curr_vsplit++]]);
                    int nbfaces = obj->getProgressiveMeshLOD()->getProgressiveMesh()->getNbFaces();
                    if (! obj->getProgressiveMeshLOD()->less())
                        break;
                    else
                    {
                        m_nbfaces -= (nbfaces - obj->getProgressiveMeshLOD()->getProgressiveMesh()->getNbFaces());
                    }
                }
            }

            for (const auto& elem : m_meshContactElements)
            {
                MeshContactElement* obj = static_cast<MeshContactElement*>(elem);
                Ra::Core::TriangleMesh newMesh;
                Ra::Core::convertPM(*(obj->getProgressiveMeshLOD()->getProgressiveMesh()->getDcel()), newMesh);
                obj->updateTriangleMesh(newMesh);
            }
        }

        void MeshContactManager::setThresholdValueChanged(int value)
        {
            Scalar th = (Scalar)(value) / PRECISION;
            displayDistribution(th, m_asymmetry);
        }

        void MeshContactManager::setAsymmetryValueChanged(int value)
        {
            Scalar as = (Scalar)(value) / PRECISION;
            displayDistribution(m_threshold, as);
        }

        void MeshContactManager::setComputeR()
        {
            distanceAsymmetryDistribution();
            LOG(logINFO) << "Distance asymmetry distributions computed.";
        }

        void MeshContactManager::setLoadDistribution(std::string filePath)
        {
            loadDistribution(filePath);
            LOG(logINFO) << "Distance asymmetry distributions loaded.";
        }


        // Display proximity zones
        void MeshContactManager::setDisplayProximities()
        {
            if (m_lambda != 0.0)
            {
                thresholdComputation(); // computing m_broader_threshold
            }

            constructPriorityQueues2();
        }

        void MeshContactManager::kmeans(int k)
        {
            // selecting k random points to be the clusters centers
            int nbPtDistrib = m_distrib.size();
            std::vector<int> indices;
            for (uint i = 0; i < nbPtDistrib; i++)
            {
                indices.push_back(i);
            }
            std::srand(std::time(0));
            std::random_shuffle(indices.begin(), indices.end());
            for (uint i = 0; i < k; i++)
            {
                std::vector<int> id;
                std::pair<Scalar, std::vector<int> > cluster;
                cluster.first = m_distrib[indices[i]].r;
                cluster.second = id;
                m_clusters.push_back(cluster);
            }

            bool stableMeans;

            do
            {
                // clustering all the faces
                for (uint i = 0; i < k; i++)
                {
                    m_clusters[i].second.clear();
                }
                for (uint i = 0; i < nbPtDistrib; i++)
                {
                    int closestCenterId = 0;
                    Scalar closestDist = abs(m_distrib[i].r - m_clusters[0].first);
                    for (uint j = 1; j <k; j++)
                    {
                        Scalar dist = abs(m_distrib[i].r - m_clusters[j].first);
                        if (dist < closestDist)
                        {
                            closestDist = dist;
                            closestCenterId = j;
                        }
                    }
                    m_clusters[closestCenterId].second.push_back(i);
                }

//                for (uint i = 0; i < k; i++)
//                {
//                    LOG(logINFO) << "Center and number of faces of cluster " << i + 1 << " : " << m_clusters[i].first << " " << m_clusters[i].second.size();
//                }

                // updating cluster centers
                stableMeans = true;
                for (uint i = 0; i < k; i++)
                {
                    Scalar meanCluster = 0;
                    int nbPtDistribCluster = m_clusters[i].second.size();
                    for (uint j = 0; j < nbPtDistribCluster; j++)
                    {
                        meanCluster += m_distrib[m_clusters[i].second[j]].r;
                    }
                    meanCluster /= nbPtDistribCluster;
                    if (m_clusters[i].first != meanCluster)
                    {
                        stableMeans = false;
                    }
                    m_clusters[i].first = meanCluster;
                }
            } while (!stableMeans);
        }

        Scalar MeshContactManager::silhouette()
        {
            Scalar S = 0;
            int nbPtDistrib = m_distrib.size();
            for (uint i = 0; i < m_clusters.size(); i++)
            {
                int nbPtDistribCluster = m_clusters[i].second.size();
                for (uint j = 0; j < nbPtDistribCluster; j++)
                {
                    Scalar a = 0;
                    for (uint k = 0; k < nbPtDistribCluster; k++)
                    {
                        if (k != j)
                        {
                            a += abs(m_distrib[m_clusters[i].second[j]].r - m_distrib[m_clusters[i].second[k]].r);
                        }
                    }
                    a /= (nbPtDistribCluster - 1);

                    Scalar b;
                    int start;
                    if (i == 0)
                    {
                        start = 1;
                    }
                    else
                    {
                        start = 0;
                    }
                    b = abs(m_distrib[m_clusters[i].second[j]].r - m_clusters[start].first);
                    for (uint k = start; k < m_clusters.size(); k++)
                    {
                        if (k != i)
                        {
                            Scalar b_cluster = abs(m_distrib[m_clusters[i].second[j]].r - m_clusters[k].first);
                            if (b_cluster < b)
                            {
                                b = b_cluster;
                            }
                        }
                    }

                    Scalar s = (b - a) / std::max(a,b);
                    S += s;
                }
            }
            S /= nbPtDistrib;
            return S;
        }

        void MeshContactManager::clustering(Scalar silhouetteMin, int nbClustersMax)
        {
            int k = 1;
            Scalar S;

            Scalar maxS = -1;

            int bestNbClusters;
            std::vector<std::pair<Scalar, std::vector<int> > > bestClusters;

            do
            {
                k++;
                kmeans(k);
                S = silhouette();

                LOG(logINFO) << "Number of clusters : " << k << " and silhouette value : " << S;

                if (S > maxS)
                {
                    maxS = S;
                    bestNbClusters = k;
                    bestClusters = m_clusters;
                }
            } while (maxS < silhouetteMin && k < nbClustersMax);

            m_clusters = bestClusters;

            LOG(logINFO) << "Best number of clusters : " << bestNbClusters << " and silhouette value : " << maxS;
        }






            Ra::Core::Vector4 vertexColor (0, 0, 0, 0);
            for (uint i = 0; i < m_meshContactElements.size(); i++)
            {
                MeshContactElement* obj = m_meshContactElements[i];
                int nbVertices = obj->getMesh()->getGeometry().m_vertices.size();
                Ra::Core::Vector4Array colors;
                for (uint v = 0; v < nbVertices; v++)
                {
                    colors.push_back(vertexColor);
                }
                obj->getMesh()->addData(Ra::Engine::Mesh::VERTEX_COLOR, colors);
            }

            {
                {
                    Ra::Core::VectorArray<Ra::Core::Triangle> t = obj->getTriangleMeshDuplicate().m_triangles;
                    Ra::Core::Vector4Array colors = obj->getMesh()->getData(Ra::Engine::Mesh::VERTEX_COLOR);

                    obj->getMesh()->addData(Ra::Engine::Mesh::VERTEX_COLOR, colors);

            }
        }

        void MeshContactManager::setDisplayClusters()
        {
            // reloading initial mesh in case of successive simplifications
            for (uint objIndex = 0; objIndex < m_meshContactElements.size(); objIndex++)
            {
                m_meshContactElements[objIndex]->setMesh(m_meshContactElements[objIndex]->getTriangleMeshDuplicate());
            }

            colorClusters();
        }

        void MeshContactManager::setConstructM0()
        {     
            m_mainqueue.clear();
            m_index_pmdata.clear();
            m_curr_vsplit = 0;

            m_nbfaces = m_nbfacesinit;

            for (uint e = 0; e < m_nbobjects; e++)
            {
                MeshContactElement* obj = m_meshContactElements[e];
                m_mainqueue.insert(obj->getPriorityQueue()->firstData());
            }

            // end criterion : number of faces set in the UI
            int i = 0;

            QueueContact::iterator it = m_mainqueue.begin();

            while (it != m_mainqueue.end() && m_nb_faces_max > m_nbfaces)
            {
                const Ra::Core::PriorityQueue::PriorityQueueData &d = *it;
                MeshContactElement* obj = static_cast<MeshContactElement*>(m_meshContactElements[d.m_index]);
                int nbfaces = obj->getProgressiveMeshLOD()->getProgressiveMesh()->getNbFaces();

                if (nbfaces > 2)
                {
                    if (edgeCollapse(obj->getIndex()))
                    {
                        m_index_pmdata.push_back(obj->getIndex());
                        m_curr_vsplit++;
                        m_nb_faces_max -= (nbfaces - obj->getProgressiveMeshLOD()->getProgressiveMesh()->getNbFaces());
                    }
                    if (obj->getPriorityQueue()->size() > 0)
                    {
                        m_mainqueue.insert(obj->getPriorityQueue()->firstData());
                    }
                    else
                    {
                        LOG(logINFO) << "Priority queue empty";
                    }
                }
                i++;
                m_mainqueue.erase(it);
                it = m_mainqueue.begin();
            }

            for (const auto& elem : m_meshContactElements)
            {
                MeshContactElement* obj = static_cast<MeshContactElement*>(elem);

                // switch from DCEL to mesh
                Ra::Core::TriangleMesh newMesh;
                Ra::Core::convertPM(*(obj->getProgressiveMeshLOD()->getProgressiveMesh()->getDcel()), newMesh);
                obj->updateTriangleMesh(newMesh);
            }
        }

        int MeshContactManager::getNbFacesMax()
        {
            computeNbFacesMax2();
            return m_nb_faces_max;
        }

        bool MeshContactManager::edgeErrorComputation(Ra::Core::HalfEdge_ptr h, int objIndex, Scalar& error, Ra::Core::Vector3& p)
        {
            const Ra::Core::Vertex_ptr& vs = h->V();
            const Ra::Core::Vertex_ptr& vt = h->Next()->V();

            MeshContactElement* obj = static_cast<MeshContactElement*>(m_meshContactElements[objIndex]);

            // test if the edge can be collapsed or if it has contact
            bool contact = false;
            Ra::Core::ProgressiveMesh<>::Primitive qk;
            Scalar dist;
            Scalar weight;
            int nbContacts = 0;
            Scalar sumWeight = 0;

            // for each edge, we look for all contacts with other objects and add the contact quadrics to the quadric of the edge
            Ra::Core::ProgressiveMesh<>::Primitive qc = Ra::Core::ProgressiveMesh<>::Primitive();
            if (m_lambda != 0.0)
            {
                for (uint k=0; k<m_trianglekdtrees.size(); k++)
                {
                    if (k != objIndex)
                    {
                        MeshContactElement* otherObj = static_cast<MeshContactElement*>(m_meshContactElements[k]);
                        std::vector<std::pair<int,Scalar> > faceIndexes;

                        // all close faces
                        obj->getProgressiveMeshLOD()->getProgressiveMesh()->edgeContacts(vs->idx, vt->idx, m_trianglekdtrees, k, std::pow(m_broader_threshold,2), faceIndexes);
                        if ( faceIndexes.size() != 0)
                        {
                            for (uint l = 0; l < faceIndexes.size(); l++)
                            {
                                dist = faceIndexes[l].second;

                                // asymmetry computation
//                                Scalar dist2 = m_distances[k][objIndex][faceIndexes[l].first].second;
//                                if (abs(dist - dist2) <= m_asymmetry)
//                                {
                                    contact = true;
                                    if (m_broader_threshold == 0.0)
                                    {
                                        CORE_ASSERT(dist == 0.0, "Contact found out of threshold limit");
                                        weight = 1;
                                    }
                                    else
                                    {
                                        CORE_ASSERT(dist/m_broader_threshold >= 0 && dist/m_broader_threshold <= 1, "Contact found out of threshold limit.");
                                        weight = std::pow(std::pow(dist/m_broader_threshold, m_m) - 1, m_n);
                                    }
                                    nbContacts++;
                                    sumWeight += weight;
                                    qk = otherObj->getFacePrimitive(faceIndexes[l].first);
                                    qk *= weight;
                                    qc += qk;
//                                }
                            }
                        }
                    }
                }
            }

            // computing the optimal placement for the resulting vertex
            Scalar edgeErrorQEM = obj->getProgressiveMeshLOD()->getProgressiveMesh()->computeEdgeError(h->idx, p);

            if (contact)
            {
                qc *= 1.0 / nbContacts;

                Scalar edgeErrorContact = abs(obj->getProgressiveMeshLOD()->getProgressiveMesh()->getEM().computeGeometricError(qc,p));
                CORE_ASSERT(error >= edgeErrorQEM, "Contacts lower the error");
            }
            else
            {
                error = edgeErrorQEM;
            }
            return contact;
        }

        void MeshContactManager::constructPriorityQueues2()
        {

            for (uint objIndex=0; objIndex < m_meshContactElements.size(); objIndex++)
            {
                Ra::Core::ProgressiveMeshBase<>* pm = new Ra::Core::ProgressiveMesh<>(&m_initTriangleMeshes[objIndex]);
                m_meshContactElements[objIndex]->setProgressiveMeshLOD(pm);
                m_meshContactElements[objIndex]->getProgressiveMeshLOD()->getProgressiveMesh()->computeFacesQuadrics();
                m_meshContactElements[objIndex]->computeFacePrimitives();
                // reloading the mesh in case of successive simplifications
                m_meshContactElements[objIndex]->setMesh(m_initTriangleMeshes[objIndex]);
            }

            for (uint objIndex=0; objIndex < m_nbobjects; objIndex++)
            {
            MeshContactElement* obj = static_cast<MeshContactElement*>(m_meshContactElements[objIndex]);
            Ra::Core::PriorityQueue pQueue = Ra::Core::PriorityQueue();
            const uint numTriangles = obj->getProgressiveMeshLOD()->getProgressiveMesh()->getDcel()->m_face.size();

            Ra::Core::Vector4 vertexColor (0, 0, 0, 0);
            int nbVertices = obj->getMesh()->getGeometry().m_vertices.size();
            Ra::Core::Vector4Array colors;
            for (uint v = 0; v < nbVertices; v++)
            {
                colors.push_back(vertexColor);
            }
            Ra::Core::Vector4 contactColor (0, 0, 1.0f, 0);

#pragma omp parallel for
            for (unsigned int i = 0; i < numTriangles; i++)
            {

                // browse edges
                Ra::Core::Vector3 p = Ra::Core::Vector3::Zero();
                int j;
                const Ra::Core::Face_ptr& f = obj->getProgressiveMeshLOD()->getProgressiveMesh()->getDcel()->m_face.at( i );
                Ra::Core::HalfEdge_ptr h = f->HE();
                for (j = 0; j < 3; j++)
                {
                    const Ra::Core::Vertex_ptr& vs = h->V();
                    const Ra::Core::Vertex_ptr& vt = h->Next()->V();

                    // to prevent adding twice the same edge
                    if (vs->idx > vt->idx)
                    {
                        h = h->Next();
                        continue;
                    }

                    Scalar error;
                    bool contact = edgeErrorComputation(h,objIndex,error, p);

                    // coloring proximity zones
                    if (contact)
                    {
                        colors[vs->idx] = contactColor;
                        colors[vt->idx] = contactColor;
                    }

                    // insert into the priority queue with the real resulting point
#pragma omp critical
                    {
                        pQueue.insert(Ra::Core::PriorityQueue::PriorityQueueData(vs->idx, vt->idx, h->idx, i, error, p, objIndex));
                    }

                    h = h->Next();
                }
            }
            obj->getMesh()->addData(Ra::Engine::Mesh::VERTEX_COLOR, colors);

            obj->setPriorityQueue(pQueue);
            }
        }

        void MeshContactManager::updatePriorityQueue2(Ra::Core::Index vsIndex, Ra::Core::Index vtIndex, int objIndex)
        {
            MeshContactElement* obj = static_cast<MeshContactElement*>(m_meshContactElements[objIndex]);
            obj->getPriorityQueue()->removeEdges(vsIndex);
            obj->getPriorityQueue()->removeEdges(vtIndex);

            Ra::Core::Vector3 p = Ra::Core::Vector3::Zero();

            // listing of all the new edges formed with vs
            Ra::Core::VHEIterator vsHEIt = Ra::Core::VHEIterator(obj->getProgressiveMeshLOD()->getProgressiveMesh()->getDcel()->m_vertex[vsIndex]);
            Ra::Core::HalfEdgeList adjHE = vsHEIt.list();

            // test if the other vertex of the edge has any contacts
            for (uint i = 0; i < adjHE.size(); i++)
            {
                Ra::Core::HalfEdge_ptr h = adjHE[i];

                const Ra::Core::Vertex_ptr& vs = h->V();
                const Ra::Core::Vertex_ptr& vt = h->Next()->V();

                Scalar error;
                edgeErrorComputation(h, objIndex, error, p);

                // insert into the priority queue with the real resulting point
                // check that the index of the starting point of the edge is smaller than the index of its ending point
                if (vs->idx < vt->idx)
                {
                    obj->getPriorityQueue()->insert(Ra::Core::PriorityQueue::PriorityQueueData(vs->idx, vt->idx, h->idx, h->F()->idx, error, p, objIndex));
                }
                else
                {
                    obj->getPriorityQueue()->insert(Ra::Core::PriorityQueue::PriorityQueueData(vt->idx, vs->idx, h->Twin()->idx, h->Twin()->F()->idx, error, p, objIndex));
                }
            }
        }

        bool MeshContactManager::edgeCollapse(int objIndex)
        {
            MeshContactElement* obj = static_cast<MeshContactElement*>(m_meshContactElements[objIndex]);

            if (obj->isConstructM0())
            {
                // edge collapse and putting the collapse data in the ProgressiveMeshLOD
                Ra::Core::PriorityQueue::PriorityQueueData d = obj->getPriorityQueue()->top();
                Ra::Core::HalfEdge_ptr he = obj->getProgressiveMeshLOD()->getProgressiveMesh()->getDcel()->m_halfedge[d.m_edge_id];

                if (he->Twin() == nullptr)
                {
                    obj->getProgressiveMeshLOD()->getProgressiveMesh()->collapseFace();
                    obj->getProgressiveMeshLOD()->oneVertexSplitPossible();
                }
                else
                {
                    obj->getProgressiveMeshLOD()->getProgressiveMesh()->collapseFace();
                    obj->getProgressiveMeshLOD()->getProgressiveMesh()->collapseFace();
                }
                obj->getProgressiveMeshLOD()->getProgressiveMesh()->collapseVertex();
                Ra::Core::ProgressiveMeshData data = Ra::Core::DcelOperations::edgeCollapse(*(obj->getProgressiveMeshLOD()->getProgressiveMesh()->getDcel()), d.m_edge_id, d.m_p_result);

                if (obj->getProgressiveMeshLOD()->getProgressiveMesh()->getNbFaces() > 0)
                {
                obj->getProgressiveMeshLOD()->getProgressiveMesh()->updateFacesQuadrics(d.m_vs_id);
                }
                // update the priority queue of the object
                // updatePriorityQueue(d.m_vs_id, d.m_vt_id, objIndex);
                updatePriorityQueue2(d.m_vs_id, d.m_vt_id, objIndex);
    //            else
    //            {
    //                while (obj->getPriorityQueue()->size() > 0)
    //                    obj->getPriorityQueue()->top();
    //            }
                obj->getProgressiveMeshLOD()->addData(data);
                obj->getProgressiveMeshLOD()->oneEdgeCollapseDone();

                return true;
            }
            else
            {
                return false;
            }
        }

        void MeshContactManager::normalize()
        {
            Ra::Core::VectorArray<Ra::Core::Vector3> v1 = m_meshContactElements[0]->getInitTriangleMesh().m_vertices;
            Super4PCS::AABB3D aabb1 = Super4PCS::AABB3D();
            for (uint a = 0; a < v1.size(); a++)
            {
                aabb1.extendTo(v1[a]);
            }
            Scalar height1 = aabb1.height();
            Scalar width1 = aabb1.width();
            Scalar depth1 = aabb1.depth();

            Ra::Core::VectorArray<Ra::Core::Vector3> v2 = m_meshContactElements[1]->getInitTriangleMesh().m_vertices;
            Super4PCS::AABB3D aabb2 = Super4PCS::AABB3D();
            for (uint a = 0; a < v2.size(); a++)
            {
                aabb2.extendTo(v2[a]);
            }
            Scalar height2 = aabb2.height();
            Scalar width2 = aabb2.width();
            Scalar depth2 = aabb2.depth();

            Scalar max = std::max(std::max(std::max(std::max(std::max(width2, depth2), height2), depth1), width1), height1);
            LOG(logINFO) << max << std::endl;

            std::string s;
            Scalar value;

            std::ifstream input("screw.obj", std::ios::in);
            CORE_ASSERT(input, "Error while opening obj file.");

            std::ofstream output("screw_normalized.obj", std::ios::out | std::ios::trunc);
            CORE_ASSERT(output, "Error while opening obj file.");

            Scalar newValue;

            while (!input.eof())
            {
                input >> s;
                if (s.compare("v") == 0)
                {
                    output << s;
                    for (uint i = 0; i < 3; i++)
                    {
                        input >> value;
                        newValue = value / max;
                        output << " " << newValue;
                    }
                    output << std::endl;
                }
                else
                {
                    output << s;
                    for (uint i = 0; i < 3; i++)
                    {
                        input >> s;
                        output << " " << s;
                    }
                    output << std::endl;
                }
            }

            output.close();
            input.close();
        }
    }
}