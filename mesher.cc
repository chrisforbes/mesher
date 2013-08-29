#include <shapefil.h>
#include <cstdio>
#include <map>
#include <cfloat>

enum mode_e {
    MODE_ELEVATION_FROM_Z = 0,
    MODE_ELEVATION_ZERO = 1,
};

struct src {
    char const * filename;
    mode_e mode;
};

void interpolate_output(double minEasting, int width, int spacing,
    std::map<double, double>* points, unsigned short *out) {

    /* output loop -- probably wants some better resampling */
    int n = 0;
    double elevation = 0;

    for (std::map<double, double>::const_iterator it = points->begin(); it != points->end(); it++) {
        int next_n = (int)((it->first - minEasting) / spacing);
        if (next_n > n) {
            double dz = (it->second - elevation) / (next_n - n);
            while (n < next_n) {
                out[n++] = elevation;
                elevation += dz;
            }
        }
    }

    while (n < width) {
        out[n++] = 0;
    }
}

#define CHUNK 512

int main(void) {

    src sources[] = {
        { "data/nz-mainland-contours-topo/nz-mainland-contours-topo-1", MODE_ELEVATION_FROM_Z },
        { "data/nz-mainland-contours-topo/nz-mainland-contours-topo-2", MODE_ELEVATION_FROM_Z },
        { "data/nz-mainland-contours-topo/nz-mainland-contours-topo-3", MODE_ELEVATION_FROM_Z },
        { "data/nz-mainland-contours-topo/nz-mainland-contours-topo-4", MODE_ELEVATION_FROM_Z },
        { "data/nz-coastlines-and-islands/nz-coastlines-and-islands", MODE_ELEVATION_ZERO },
        { 0, MODE_ELEVATION_ZERO },
    };

    double spacing = 10;

    double minEasting = 0.0, maxEasting = 0.0;
    double minNorthing = 0.0, maxNorthing = 0.0;

    for (src *s = sources; s->filename; s++) {
        SHPHandle sh = SHPOpen(s->filename, "rb");
        double mins[4];
        double maxs[4];

        SHPGetInfo(sh, 0, 0, mins, maxs);
        if (mins[0] < minEasting || minEasting == 0.0)
            minEasting = mins[0];
        if (mins[1] < minNorthing || minNorthing == 0.0)
            minNorthing = mins[1];
        if (maxs[0] > maxEasting || maxEasting == 0.0)
            maxEasting = maxs[0];
        if (maxs[1] > maxNorthing || maxNorthing == 0.0)
            maxNorthing = maxs[1];

        SHPClose(sh);
    }

    int heightmap_width = (int)(((maxEasting - minEasting) + spacing - 1) / spacing);
    int heightmap_height = (int)(((maxNorthing - minNorthing) + spacing - 1) / spacing);

    unsigned short * out = new unsigned short[heightmap_width];

    FILE * f = fopen("out.dat", "wb");
    fwrite(&heightmap_width, sizeof(int), 1, f);
    fwrite(&heightmap_height, sizeof(int), 1, f);

    for (int slice = 0; slice < heightmap_height; slice+=CHUNK) {

        double latMetric = minNorthing + slice * spacing;
        printf("Slice %d/%d                     \n",
            slice, heightmap_height);

        std::map<double, double> points[CHUNK];

        for (src *s = sources; s->filename; s++) {
            SHPHandle sh = SHPOpen(s->filename, "rb");

            int num_entities;
            SHPGetInfo(sh, &num_entities, 0, 0, 0);

            for (int i = 0; i < num_entities; i++) {
                SHPObject * o = SHPReadObject(sh, i);

                if (o->dfYMin <= latMetric && o->dfYMax >= latMetric + CHUNK * spacing) {
                    double elevation = s->mode == MODE_ELEVATION_FROM_Z ? o->dfZMax : 0.0;

                    /* poly lines -- if we need to do loops, more work. */
                    double last_y = o->padfY[0];
                    double last_x = o->padfX[0];

                    for (int v = 1; v < o->nVertices; v++) {
                        double this_y = o->padfY[v];
                        double this_x = o->padfX[v];

                        for (int q = 0; q < CHUNK && slice+q < heightmap_height; q++) {
                            double m = latMetric + q * spacing;
                            if ((last_y <= m) ^ (this_y <= m)) {
                                /* this line segment crosses the cross-section */
                                /* t * last_y + (1 - t) * this_y = latMetric */
                                double t = (m - this_y) / (last_y - this_y);
                                double x = t * last_x + (1 - t) * this_x;

                                points[q][x] = elevation;
                            }
                        }

                        last_y = this_y;
                        last_x = this_x;
                    }
                }

                SHPDestroyObject(o);
            }

            SHPClose(sh);
        }

        for (int q = 0; q < CHUNK && slice+q < heightmap_height; q++) {
            interpolate_output(minEasting, heightmap_width, spacing, &points[q], out);
            fwrite(out, sizeof(unsigned short), heightmap_width, f);
        }
    }

    fclose(f);
}
