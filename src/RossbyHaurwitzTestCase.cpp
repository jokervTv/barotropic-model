#include "RossbyHaurwitzTestCase.h"

namespace barotropic_model {

RossbyHaurwitzTestCase::RossbyHaurwitzTestCase() {
    R = 4;
    omega = 3.924e-6;
    phi0 = G*8e3;
    REPORT_ONLINE;
}

RossbyHaurwitzTestCase::~RossbyHaurwitzTestCase() {
    REPORT_OFFLINE;
}

/**
 *  u = aω (cosφ + R sin²φ cosᴿ⁻¹φ cosRλ - cosᴿ⁺¹φ sinφ cosRλ)
 *
 *  v = - aωR cosᴿ⁻¹φ sinφ sinRλ
 *
 *  h = gh0 + a²A(φ) + a²B(φ)cosRλ + a²C(φ)cos2Rλ
 *
 */
void RossbyHaurwitzTestCase::
calcInitCond(BarotropicModel &model) {
    if (dynamic_cast<const geomtk::SphereDomain*>(&model.domain()) == NULL) {
        REPORT_ERROR("Rossby-Haurwitz test case is only valid in sphere domain!");
    }
    TimeLevelIndex<2> initTimeIdx;
    const Mesh &mesh = model.mesh();
    Field<double, 2> &u = model.zonalWind();
    Field<double, 2> &v = model.meridionalWind();
    Field<double, 2> &gd = model.geopotentialDepth();
    Field<double> &ghs = model.surfaceGeopotential();
    double Re = model.domain().radius();
    double R2 = R*R;
    double R_1 = R+1;
    double R_2 = R+2;
    double omega2 = omega*omega;
    int js = mesh.js(FULL), jn = mesh.je(FULL);
    // -------------------------------------------------------------------------
    // zonal wind speed
    for (int j = mesh.js(u.gridType(1)); j <= mesh.je(u.gridType(1)); ++j) {
        if (u.gridType(1) == FULL && (j == js || j == jn)) {
            continue;
        }
        double cosLat = mesh.cosLat(u.gridType(1), j);
        double sinLat = mesh.sinLat(u.gridType(1), j);
        double cosLatR = pow(cosLat, R);
        for (int i = mesh.is(u.gridType(0)); i <= mesh.ie(u.gridType(0)); ++i) {
            double lon = mesh.gridCoordComp(0, u.gridType(0), i);
            double cosRLon = cos(R*lon);
            double a = cosLat;
            double b = cosLatR/cosLat*sinLat*sinLat*cosRLon*R;
            double c = -cosLatR*cosLat*cosRLon;
            u(initTimeIdx, i, j) = (a+b+c)*Re*omega;
        }
    }
    // -------------------------------------------------------------------------
    // meridional wind speed
    for (int j = mesh.js(v.gridType(1)); j <= mesh.je(v.gridType(1)); ++j) {
        if (v.gridType(1) == FULL && (j == js || j == jn)) {
            continue;
        }
        double cosLat = mesh.cosLat(v.gridType(1), j);
        double sinLat = mesh.sinLat(v.gridType(1), j);
        double cosLatR = pow(cosLat, R);
        for (int i = mesh.is(v.gridType(0)); i <= mesh.ie(v.gridType(0)); ++i) {
            double lon = mesh.gridCoordComp(0, v.gridType(0), i);
            double sinRLon = sin(R*lon);
            v(initTimeIdx, i, j) = -Re*omega*R*cosLatR/cosLat*sinLat*sinRLon;
        }
    }
    // -------------------------------------------------------------------------
    // surface geopotential height and geopotential depth
    assert(gd.staggerLocation() == CENTER);
    for (int j = mesh.js(FULL)+1; j<= mesh.je(FULL)-1; ++j) {
        double cosLat = mesh.cosLat(FULL, j);
        double cosLat2 = cosLat*cosLat;
        double cosLatR = pow(cosLat, R);
        double cosLatR2 = cosLatR*cosLatR;
        double a = (omega*OMEGA+0.5*omega2)*cosLat2+0.25*omega2*cosLatR2*(R_1*cosLat2+(2*R2-R-2)-2*R2/cosLat2);
        double b = 2*(omega*OMEGA+omega2)*cosLatR*((R2+2*R+2)-R_1*R_1*cosLat2)/R_1/R_2;
        double c = 0.25*omega2*cosLatR2*(R_1*cosLat2-R_2);
        for (int i = mesh.is(FULL); i <= mesh.ie(FULL); ++i) {
            double lon = mesh.gridCoordComp(0, FULL, i);
            double cosRLon = cos(R*lon);
            double cos2RLon = cos(2*R*lon);
            gd(initTimeIdx, i, j) = phi0+Re*Re*(a+b*cosRLon+c*cos2RLon);
            ghs(i, j) = 0;
        }
    }
    // -------------------------------------------------------------------------
    // set Poles
    for (int i = mesh.is(FULL); i <= mesh.ie(FULL); ++i) {
        if (u.gridType(1) == FULL) {
            u(initTimeIdx, i, js) = 0;
            u(initTimeIdx, i, jn) = 0;
        }
        if (v.gridType(1) == FULL) {
            v(initTimeIdx, i, js) = 0;
            v(initTimeIdx, i, jn) = 0;
        }
        gd(initTimeIdx, i, js) = phi0;
        gd(initTimeIdx, i, jn) = phi0;
        ghs(i, js) = 0;
        ghs(i, jn) = 0;
        
    }
#ifndef NDEBUG
    assert(gd.min(initTimeIdx) != 0.0);
#endif
    // -------------------------------------------------------------------------
    u.applyBndCond(initTimeIdx);
    v.applyBndCond(initTimeIdx);
    gd.applyBndCond(initTimeIdx);
    ghs.applyBndCond();
}

}
