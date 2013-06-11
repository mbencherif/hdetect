/*****************************************************************
 *
 * This file is part of the People2D project
 *
 * People2D Copyright (c) 2011 Luciano Spinello
 *
 * This software is licensed under the "Creative Commons 
 * License (Attribution-NonCommercial-ShareAlike 3.0)" 
 * and is copyrighted by Luciano Spinello
 * 
 * Further information on this license can be found at:
 * http://creativecommons.org/licenses/by-nc-sa/3.0/
 * 
 * People2D is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied 
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  
 *
 *****************************************************************/

#include "upm/lib/lengine.hpp"



// ---------------------------------------------------------
void lengine::set_featureset()
{
  std::vector<int> featnum;
  std::vector<int> featnum_comparative;
  //~ default mix
  if (params.featuremix == 0)
  {
    //printf("Standard features mix\n");
    featnum.push_back(0);
    featnum.push_back(1);
    featnum.push_back(2);
    featnum.push_back(3);
    featnum.push_back(4);
    featnum.push_back(5);
    featnum.push_back(6);
    featnum.push_back(7);
    featnum.push_back(8);
    featnum.push_back(9);
    featnum.push_back(10);
    featnum.push_back(11);
    featnum.push_back(12);
    featnum.push_back(13);
    featnum.push_back(14);
    featnum.push_back(15);
    featnum_comparative.push_back(0);
  }

  //~ no distance
  if (params.featuremix == 1)
  {
    printf("Features mix 1\n");
    featnum.push_back(0);
    featnum.push_back(1);
    featnum.push_back(2);
    featnum.push_back(3);
    featnum.push_back(4);
    featnum.push_back(5);
    featnum.push_back(6);
    featnum.push_back(7);
    featnum.push_back(8);
    featnum.push_back(9);
    featnum.push_back(10);
    featnum.push_back(11);
    featnum.push_back(12);
    featnum.push_back(13);
    featnum.push_back(14);
    featnum_comparative.push_back(0);
  }

  //~ no distance no relational
  if (params.featuremix == 2)
  {
    printf("Features mix 2\n");
    featnum.push_back(0);
    featnum.push_back(1);
    featnum.push_back(2);
    featnum.push_back(3);
    featnum.push_back(4);
    featnum.push_back(5);
    featnum.push_back(6);
    featnum.push_back(7);
    featnum.push_back(8);
    featnum.push_back(9);
    featnum.push_back(10);
    featnum.push_back(11);
    featnum.push_back(12);
    featnum.push_back(13);
    featnum.push_back(14);
  }

  lfeatures = new LSL_lfeatures_class(featnum, featnum_comparative);
  fsz = featnum.size() + featnum_comparative.size();

}
// ---------------------------------------------------------

int lengine::sanity_check(std::vector<std::vector<Real> > &descriptor)
{
  int ret = 1;
  if (!descriptor.size())
  {
    printf("Feature set is 0 dimensional\n");
    return (0);
  }

  unsigned int f_num = descriptor[0].size();

  for (unsigned int i = 0; i < descriptor.size(); i++)
  {
    if (f_num != descriptor[i].size())
    {
      printf("Feature size mismatch [%d]\n", i);
      ret = 0;
    }

    for (unsigned int j = 0; j < descriptor[i].size(); j++)
    {
      int typeval = fpclassify(descriptor[i][j]);
      if (typeval == FP_NAN || typeval == FP_INFINITE)
      {
        printf("nan or inf found in the feature @ position [%d][%d] = %g\n", i, j, descriptor[i][j]);
        ret = 0;
      }
    }
  }

  return ret;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int lengine::get_breakpoint(std::vector<LSL_Point3D_str> &pts, int last_breaking_idx, double sqjumpdist)
{
  int ptsz = pts.size() - 1;

  //~ failsafe
  int jmp_idx = pts.size();

  for (int i = last_breaking_idx; i < ptsz; i++)
  {
    double dist;
    dist = distance_L2_XY_sqr(&pts[i], &pts[i + 1]);

    if (dist > sqjumpdist)
    {
      //~ printf("dist: %g  (%g): [%g %g]v[%g %g]\n",dist, sqjumpdist, pts[i].x,pts[i].y, pts[i + 1].x, pts[i + 1].y);

      //~ mark index
      jmp_idx = i + 1;

      //~ bail out
      i = ptsz;
    }
  }

  return (jmp_idx);
}


int lengine::segmentscanJDC(std::vector<LSL_Point3D_container> &clusters)
{

  //~ bailing var
  char split_complete = 1;

  //~ numpts
  int ptsz = laserscan_single.data.pts.size();

  //~ avoid nulls
  if (ptsz == 0)
    return (0);

  //~ last break point index
  int last_breaking_idx = 0;

  LSL_Point3D_str single_cog;
  std::vector<LSL_Point3D_str> all_cog;
  uint i;
  double dist[5];

  uint clusterNo = 0;

  bool newcluster;
  while (split_complete)
  {
    //~ min pts number
    if (last_breaking_idx < ptsz - 1)
    {
      //~ max distance check
      int breaking_idx = get_breakpoint(laserscan_single.data.pts, last_breaking_idx, params.sqjumpdist);

      if (breaking_idx - last_breaking_idx >= L_MINCLUSTERSZ)
      {
        //~ a cluster
        LSL_Point3D_container single_cluster;

        //~ pump it into
        single_cluster.pts.insert(single_cluster.pts.begin(), laserscan_single.data.pts.begin() + last_breaking_idx,
                                  laserscan_single.data.pts.begin() + breaking_idx);

        //printf("\n\n--- NEW CLUSTER COMPUTED ---");
        //printf("current cluster no %d\n", clusterNo);
        //printf("total clusters no %d\n", all_cog.size());

        newcluster = true;
        //concatenate clusters
        //compute the cog
        single_cluster.compute_cog(&single_cog);
        //compare it with all existing cogs
        for (i = 0; i < all_cog.size(); i++)
        {
          //we compute the distance between each end of both clusters
          // and the distance between the cogs
          // if the minimum of these distances is below a threshold, we concatenate the clusters
          dist[0] = distance_L2_XY(&clusters[i].pts.front(), &single_cluster.pts.front());
          dist[1] = distance_L2_XY(&clusters[i].pts.front(), &single_cluster.pts.back());
          dist[2] = distance_L2_XY(&clusters[i].pts.back(), &single_cluster.pts.front());
          dist[3] = distance_L2_XY(&clusters[i].pts.back(), &single_cluster.pts.back());
          dist[4] = distance_L2_XY(&single_cog, &all_cog[i]);
          //printf("distances: %f\t%f\t%f\t%f\t%f\n", dist[0],dist[1],dist[2],dist[3],dist[4]);
          if (*std::min_element(dist, dist + 5) < params.dseg) // half of the segmentation distance
          //if (dist[4] <  params.dseg) // half of the segmentation distance
          { //printf("close cluster %d\n", i);
            newcluster = false;
            while (!single_cluster.pts.empty())
            {
              clusters[i].pts.push_back(single_cluster.pts.back());
              single_cluster.pts.pop_back();
            }
            break;
          }
        }
        //printf("\n");
        if (newcluster)
        {
          all_cog.push_back(single_cog);
          clusters.push_back(single_cluster);
        }
        clusterNo++;

      }

      //~ endpoint
      last_breaking_idx = breaking_idx;
    }
    else
    {
      //~ break cycle
      split_complete = 0;
      //printf("current cluster no %d\n", clusterNo);
      //printf("[PEOPLE2D_ENGINE] Connected clusters %d\n", all_cog.size()-clusterNo);
    }
  }

  return (1);
}

void lengine::computeFeatures(std::vector<LSL_Point3D_container> &clusters,
                                      std::vector<std::vector<float> > &descriptor)
{
  // set feature set
  set_featureset();

  lfeatures->compute_descriptor(clusters, descriptor);

  if (params.sanity)
  {
    int ret = sanity_check(descriptor);
    if (!ret)
    {
      printf("Sanity check failed.\n");
      exit(1);
    }
    else
    {
      if (params.verbosity == 2)
        printf("Sanity check passed \n");
    }
  }
}