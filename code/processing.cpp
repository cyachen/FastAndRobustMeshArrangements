#include "processing.h"



inline double computeMultiplier(const std::vector<double> &coords)
{
    const double R = 11259470696.0; //avg_max_coord (167.78) * old_multiplier (67108864.0)

    double max_coord = *std::max_element(coords.begin(), coords.end());
    double min_coord = *std::min_element(coords.begin(), coords.end());

    double abs_max_coord = std::max(std::abs(min_coord), std::abs(max_coord));

    double div = R / abs_max_coord;

    //closest power of 2
    uint n = static_cast<uint>(div);
    uint pow_next, pow_prev, count = 0;

    if (n && !(n & (n - 1)))
            pow_next = n;
    else
    {
        while(n != 0)
        {
            n >>= 1;
            count += 1;
        }

        pow_next = 1 << count;
    }

    pow_prev = pow_next / 2;

    double d_prev = div - pow_prev;
    double d_next = pow_next - div;

    return (d_prev < d_next) ? pow_prev : pow_next;
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void mergeDuplicatedVertices(const std::vector<double> &in_coords, const std::vector<uint> &in_tris,
                                    std::vector<genericPoint*> &verts, std::vector<uint> &tris)
{
    verts.reserve(in_coords.size() / 3);
    tris.reserve(in_tris.size());

    std::map<std::vector<double>, uint> v_map;

    for(const uint &v_id : in_tris)
    {
        std::vector<double> v = {in_coords[(3 * v_id)], in_coords[(3 * v_id) +1], in_coords[(3 * v_id) +2]};

        auto ins = v_map.insert({v, v_map.size()});
        if(ins.second)
            verts.push_back(new explicitPoint3D(v[0], v[1], v[2])); // new vtx added

        tris.push_back(ins.first->second);
    }
}


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

inline void removeDegenerateAndDuplicatedTriangles(const std::vector<genericPoint*> &verts, const std::vector<std::bitset<NBIT> > &in_labels,
                                                   std::vector<uint> &tris, std::vector< std::bitset<NBIT> > &labels)
{
    labels = in_labels;

    uint num_orig_tris = static_cast<uint>(tris.size() / 3);
    uint t_off = 0;
    uint l_off = 0;

    std::map< std::vector<uint>, uint> tris_map;

    for(uint t_id = 0; t_id < num_orig_tris; t_id++)
    {
        uint v0_id = tris[(3 * t_id)];
        uint v1_id = tris[(3 * t_id) +1];
        uint v2_id = tris[(3 * t_id) +2];
        std::bitset<NBIT> l = labels[t_id];

        if(!cinolib::points_are_colinear_3d(verts[v0_id]->toExplicit3D().ptr(),
                                            verts[v1_id]->toExplicit3D().ptr(),
                                            verts[v2_id]->toExplicit3D().ptr())) // good triangle
        {
            std::vector<uint> tri = {v0_id, v1_id, v2_id};
            std::sort(tri.begin(), tri.end());

            auto ins = tris_map.insert({tri, l_off});

            if(ins.second) // first time for tri v0, v1, v2
            {
                labels[l_off] = l;
                l_off++;

                tris[t_off]    = v0_id;
                tris[t_off +1] = v1_id;
                tris[t_off +2] = v2_id;
                t_off += 3;
            }
            else
            {
                uint pos = ins.first->second;
                labels[pos] |= l; // label for duplicates
            }
        }
    }

    tris.resize(t_off);
    labels.resize(l_off);
}


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

void freePointsMemory(std::vector<genericPoint *> &points)
{
    for(uint p = 0; p < points.size(); p++)
        delete points[p];
}

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

void computeApproximateCoordinates(const std::vector<genericPoint *> &vertices, std::vector<double> &coords)
{
    coords.reserve(3 * (vertices.size() -5));
    double multiplier = vertices.back()->toExplicit3D().X();

    for(uint i = 0; i < (vertices.size() - 5); i++)
    {
        auto &v = vertices[i];

        if(v->isExplicit3D())
        {
            coords.push_back(v->toExplicit3D().X() / multiplier);
            coords.push_back(v->toExplicit3D().Y() / multiplier);
            coords.push_back(v->toExplicit3D().Z() / multiplier);
        }
        else //implicit point
        {
            double x, y, z;
            v->getApproxXYZCoordinates(x, y, z);
            coords.push_back(x / multiplier);
            coords.push_back(y / multiplier);
            coords.push_back(z / multiplier);
        }
    }
}


