/*
    Copyright 2016 Thibault Vatter, Thomas Nagler

    This file is part of vinecopulib.

    vinecopulib is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    vinecopulib is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with vinecopulib.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "bicop_gumbel.hpp"
#include <cmath>

// constructor
GumbelBicop::GumbelBicop()
{
    family_ = 4;
    rotation_ = 0;
    association_direction_ = "positive";
    parameters_ = VecXd::Ones(1);
    parameters_bounds_ = MatXd::Ones(1, 2);
    parameters_bounds_(0, 1) = 200.0;
}

GumbelBicop::GumbelBicop(const VecXd& parameters)
{
    family_ = 4;
    rotation_ = 0;
    association_direction_ = "positive";
    parameters_ = parameters;
    parameters_bounds_ = MatXd::Ones(1, 2);
    parameters_bounds_(0, 1) = 200.0;
}

GumbelBicop::GumbelBicop(const VecXd& parameters, const int& rotation)
{
    family_ = 4;
    rotation_ = rotation;
    if ((rotation == 90) | (rotation == 270))
        association_direction_ = "negative";
    else
        association_direction_ = "positive";
    parameters_ = parameters;
    parameters_bounds_ = MatXd::Ones(1, 2);
    parameters_bounds_(0, 1) = 200.0;
}

VecXd GumbelBicop::generator(const VecXd& u)
{
    double theta = double(this->parameters_(0));
    VecXd psi = u;
    psi = psi.cwiseInverse().array().log().pow(theta);
    return psi;
}
VecXd GumbelBicop::generator_inv(const VecXd& u)
{
    double theta = double(this->parameters_(0));
    VecXd psi = u;
    psi = (-1.0) * psi.array().pow(1/theta);
    psi = psi.array().exp();
    return psi;
}

VecXd GumbelBicop::generator_derivative(const VecXd& u)
{
    double theta = double(this->parameters_(0));
    VecXd psi = u;
    psi = psi.cwiseInverse().array().log().pow(theta - 1.0);
    psi = (-theta) * psi.cwiseQuotient(u);
    return psi;
}

VecXd GumbelBicop::generator_derivative2(const VecXd& u)
{
    double theta = double(this->parameters_(0));;
    VecXd psi_ilog = u.cwiseInverse().array().log();
    VecXd psi = (theta - 1.0) * VecXd::Ones(u.size()) + psi_ilog;
    psi_ilog = psi_ilog.array().pow(theta - 2.0);
    psi = theta*psi.cwiseProduct(psi_ilog);
    psi_ilog = u.array().square();
    psi = psi.cwiseQuotient(psi_ilog);
    return psi;
}

VecXd GumbelBicop::hinv(const MatXd& u)
{
    double theta = double(this->parameters_(0));
    double u1, u2;
    VecXd hinv = VecXd::Zero(u.rows());
    for (int j = 0; j < u.rows(); ++j) {
        u1 = u(j, 1);
        u2 = u(j, 0);
        hinv(j) = qcondgum(&u1, &u2, &theta);
    }

    return hinv;
}

// link between Kendall's tau and the par_bicop parameter
VecXd GumbelBicop::tau_to_parameters(const double& tau)
{
    return VecXd::Constant(1, 1.0 / (1 - std::fabs(tau)));
}

double GumbelBicop::parameters_to_tau(const VecXd& parameters)
{
    double tau = (parameters(0) - 1) / parameters(0);
    if ((rotation_ == 90) | (rotation_ == 270))
        tau *= -1;

    return tau;
}

// This is copy&paste from the VineCopula package
double qcondgum(double* q, double* u, double* de)
{
    double a,p,g,gp,z1,z2,con,de1,dif;
    double mxdif;
    int iter;

    p = 1-*q;
    z1 = -log(*u);
    con=log(1.-p)-z1+(1.-*de)*log(z1); de1=*de-1.;
    a=pow(2.*pow(z1,*de),1./(*de));
    mxdif=1; iter=0;
    dif=.1;  // needed in case first step leads to NaN
    while ((mxdif > 1.e-6) && (iter < 20))
    {
        g=a+de1*log(a)+con;
        gp=1.+de1/a;
        if (std::isnan(g) || std::isnan(gp) || std::isnan(g/gp) ) {
            // added for de>50
            dif/=-2.;
        } else {
            dif=g/gp;
        }
        a-=dif; iter++;
        int it = 0;
        while ((a <= z1) && (it < 20)) {
            dif /= 2.;
            a += dif;
            ++it;
        }
        mxdif=fabs(dif);
    }
    z2=pow(pow(a,*de)-pow(z1,*de),1./(*de));
    return(exp(-z2));
}
