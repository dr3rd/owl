/*
 * OWL - an OCaml numerical library for scientific computing
 * Copyright (c) 2016-2017 Liang Wang <liang.wang@cl.cam.ac.uk>
 */

#include "owl_random.h"

// FIXME: currently in owl_common_c.c file.
// Internal state of SFMT PRNG
// sfmt_t sfmt_state;


double rng_std_gamma(double shape) {
  double b, c;
  double U, V, X, Y;

  if (shape == 1.0)
    return rng_std_exp();
  else if (shape < 1.0) {
    for ( ; ; ) {
      U = sfmt_f64_3;
      V = rng_std_exp();
      if (U <= 1.0 - shape) {
        X = pow(U, 1. / shape);
        if (X <= V)
          return X;
      }
      else {
        Y = -log((1 - U) / shape);
        X = pow(1.0 - shape + shape * Y, 1./ shape);
        if (X <= (V + Y))
          return X;
      }
    }
  }
  else {
    b = shape - 1. / 3.;
    c = 1. / sqrt(9 * b);
    for ( ; ; ) {
      do {
        X = f64_gaussian;
        V = 1.0 + c * X;
      } while (V <= 0.0);

      V = V*V*V;
      U = sfmt_f64_3;
      if (U < 1.0 - 0.0331*(X*X)*(X*X)) return (b*V);
      if (log(U) < 0.5*X*X + b*(1. - V + log(V))) return (b*V);
    }
  }
}


double rng_gamma(double shape, double scale) {
  return scale * rng_std_gamma(shape);
}


double rng_beta(double a, double b)
{
  double Ga, Gb;

  if ((a <= 1.) && (b <= 1.)) {
    double U, V, X, Y;

    while (1) {
      U = sfmt_f64_3;
      V = sfmt_f64_3;
      X = pow(U, 1. / a);
      Y = pow(V, 1. / b);

      if ((X + Y) <= 1.0) {
        if (X +Y > 0)
          return X / (X + Y);
        else {
          double logX = log(U) / a;
          double logY = log(V) / b;
          double logM = logX > logY ? logX : logY;
          logX -= logM;
          logY -= logM;
          return exp(logX - log(exp(logX) + exp(logY)));
        }
      }
    }
  }
  else {
    Ga = rng_std_gamma(a);
    Gb = rng_std_gamma(b);
    return Ga / (Ga + Gb);
  }
}


long rng_poisson_mult(double lam) {
  long X = 0;
  double enlam = exp(-lam);
  double prod = 1.;
  while (1) {
    prod *= sfmt_f64_3;
    if (prod > enlam)
      X += 1;
    else
      return X;
  }
}


static double loggam(double x) {
  static double a[10] = {
    8.333333333333333e-02,-2.777777777777778e-03,
    7.936507936507937e-04,-5.952380952380952e-04,
    8.417508417508418e-04,-1.917526917526918e-03,
    6.410256410256410e-03,-2.955065359477124e-02,
    1.796443723688307e-01,-1.39243221690590e+00 };
  double x0 = x;
  long n = 0;
  if ((x == 1.) || (x == 2.))
    return 0.;
  else if (x <= 7.) {
    n = (long) (7 - x);
    x0 = x + n;
  }
  double x2 = 1. / (x0 * x0);
  double xp = 2 * M_PI;
  double gl0 = a[9];
  for (long k = 8; k >= 0; k--) {
    gl0 *= x2;
    gl0 += a[k];
  }
  double gl = gl0 / x0 + 0.5 * log(xp) + (x0 - 0.5) * log(x0) - x0;
  if (x <= 7.0) {
    for (long k = 1; k <= n; k++) {
      gl -= log(x0 - 1.0);
      x0 -= 1.0;
    }
  }
  return gl;
}


long rng_poisson_ptrs(double lam) {
  long k;
  double U, V, slam, loglam, a, b, invalpha, vr, us;

  slam = sqrt(lam);
  loglam = log(lam);
  b = 0.931 + 2.53 * slam;
  a = -0.059 + 0.02483 * b;
  invalpha = 1.1239 + 1.1328 / (b - 3.4);
  vr = 0.9277 - 3.6224 / (b - 2);

  while (1) {
    U = sfmt_f64_3 - 0.5;
    V = sfmt_f64_3;
    us = 0.5 - fabs(U);
    k = (long) floor((2 * a / us + b) * U + lam + 0.43);
    if ((us >= 0.07) && (V <= vr))
      return k;
    if ((k < 0) || ((us < 0.013) && (V > us)))
      continue;
    if ((log(V) + log(invalpha) - log(a / (us * us) + b)) <=
        (-lam + k * loglam - loggam(k + 1)))
      return k;
  }
}


long rng_poisson(double lam)
{
  if (lam == 0)
    return 0;
  else if (lam >= 10)
    return rng_poisson_ptrs(lam);
  else
    return rng_poisson_mult(lam);
}


double rng_std_cauchy() {
  return rng_std_gaussian() / rng_std_gaussian();
}


double rng_std_t(double df) {
  double N = rng_std_gaussian();
  double G = rng_std_gamma(df / 2);
  return (sqrt(df / 2) * N / sqrt(G));
}


double rng_vonmises(double mu, double kappa) {
  double s;
  double U, V, W, Y, Z;
  double result, mod;
  int neg;

  if (kappa < 1e-8)
    return M_PI * (2 * sfmt_f64_3 - 1);
  else {
    /* with double precision rho is zero until 1.4e-8 */
    if (kappa < 1e-5)
      s = (1./kappa + kappa);
    else {
      double r = 1 + sqrt(1 + 4 * kappa * kappa);
      double rho = (r - sqrt(2 * r)) / (2 * kappa);
      s = (1 + rho * rho) / (2 * rho);
    }

    while (1) {
      U = sfmt_f64_3;
      Z = cos(M_PI * U);
      W = (1 + s * Z) / (s + Z);
      Y = kappa * (s - W);
      V = sfmt_f64_3;
      if ((Y * (2 - Y) - V >= 0) || (log(Y / V) + 1 - Y >= 0))
        break;
    }

    U = sfmt_f64_3;

    result = acos(W);
    if (U < 0.5)
      result = -result;
    result += mu;
    neg = (result < 0);
    mod = fabs(result);
    mod = (fmod(mod + M_PI, 2 * M_PI) - M_PI);
    if (neg)
      mod *= -1;

    return mod;
  }
}


double rng_pareto(double a) {
  return exp(rng_std_exp() / a) - 1.;
}


double rng_weibull(double a) {
  return pow(rng_std_exp(), 1. / a);
}


double rng_power(double a) {
  return pow(1. - exp(-rng_std_exp()), 1. / a);
}


double rng_laplace(double loc, double scale) {
  double U = sfmt_f64_3;

  if (U < 0.5)
    U = loc + scale * log(U + U);
  else
    U = loc - scale * log(2. - U - U);
  return U;
}


double rng_gumbel(double loc, double scale) {
  double U = 1. - sfmt_f64_3;
  return loc - scale * log(-log(U));
}


double rng_logistic(double loc, double scale) {
  double U = sfmt_f64_3;
  return loc + scale * log(U/(1. - U));
}


double rng_lognormal(double mu, double sigma) {
  return exp(rng_gaussian(mu, sigma));
}


double rng_rayleigh(double mode) {
  return mode * sqrt(-2. * log(1. - sfmt_f64_3));
}


double rng_wald(double mu, double lambda) {
  double mu_2l = mu / (2 * lambda);
  double Y = rng_std_gaussian();
  Y = mu * Y * Y;
  double X = mu + mu_2l * (Y - sqrt(4 * lambda * Y + Y * Y));
  double U = sfmt_f64_3;
  if (U <= mu / (mu + X))
    return X;
  else
    return (mu * mu / X);
}


long rng_zipf(double a) {
  double am1 = a - 1.;
  double b = pow(2., am1);

  while (1) {
    double U = 1. - sfmt_f64_3;
    double V = sfmt_f64_3;
    double X = floor(pow(U, -1. / am1));

    if (X > LONG_MAX || X < 1.0)
      continue;

    double T = pow(1. + 1. / X, am1);
    if (V * X * (T - 1.) / (b - 1.) <= T / b)
        return (long) X;
  }
}


long rng_geometric_search(double p) {
  long X = 1;
  double sum = p;
  double prod = p;
  double q = 1. - p;
  double U = sfmt_f64_3;
  while (U > sum) {
    prod *= q;
    sum += prod;
    X++;
  }
  return X;
}


long rng_geometric_inversion(double p) {
  return (long) ceil(log(1. - sfmt_f64_3) / log(1. - p));
}


long rng_geometric(double p) {
  if (p >= 0.333333333333333333333333)
    return rng_geometric_search(p);
  else
    return rng_geometric_inversion(p);
}


long rng_hypergeometric_hyp(long good, long bad, long sample) {
  long d1 = bad + good - sample;
  double d2 = good < bad ? good : bad;

  double Y = d2;
  long K = sample;
  while (Y > 0.0) {
    Y -= (long) floor(sfmt_f64_3 + Y / (d1 + K));
    K--;
    if (K == 0) break;
  }
  long Z = d2 - Y;
  if (good > bad) Z = sample - Z;
  return Z;
}


#define D1 1.7155277699214135  /* D1 = 2*sqrt(2/e) */
#define D2 0.8989161620588988  /* D2 = 3 - 2*sqrt(3/e) */
long rng_hypergeometric_hrua(long good, long bad, long sample) {
  long mingoodbad, maxgoodbad, popsize, m, d9;
  double d4, d5, d6, d7, d8, d10, d11;
  long Z;
  double T, W, X, Y;

  mingoodbad = fmin(good, bad);
  popsize = good + bad;
  maxgoodbad = fmax(good, bad);
  m = fmin(sample, popsize - sample);
  d4 = ((double) mingoodbad) / popsize;
  d5 = 1. - d4;
  d6 = m * d4 + 0.5;
  d7 = sqrt((double) (popsize - m) * sample * d4 * d5 / (popsize - 1) + 0.5);
  d8 = D1 * d7 + D2;
  d9 = (long) floor((double) (m + 1) * (mingoodbad + 1) / (popsize + 2));
  d10 = (loggam(d9 + 1) + loggam(mingoodbad - d9 + 1) + loggam(m - d9 + 1) +
         loggam(maxgoodbad - m + d9 + 1));
  d11 = fmin(fmin(m, mingoodbad) + 1., floor(d6 + 16 * d7));

  while (1) {
    X = sfmt_f64_3;
    Y = sfmt_f64_3;
    W = d6 + d8 * (Y - 0.5) / X;

    if ((W < 0.) || (W >= d11)) continue;

    Z = (long) floor(W);
    T = d10 - (loggam(Z + 1) + loggam(mingoodbad - Z + 1) + loggam(m - Z + 1) +
               loggam(maxgoodbad - m + Z + 1));

    if ((X * (4. - X) - 3.) <= T) break;
    if (X * (X - T) >= 1) continue;
    if (2. * log(X) <= T) break;
  }

  /* this is a correction to HRUA* by Ivan Frohne in rv.py */
  if (good > bad) Z = m - Z;
  /* another fix from rv.py to allow sample to exceed popsize/2 */
  if (m < sample) Z = good - Z;

  return Z;
}
#undef D1
#undef D2


long rnbg_hypergeometric(long good, long bad, long sample) {
  if (sample > 10)
    return rng_hypergeometric_hrua(good, bad, sample);
  else
    return rng_hypergeometric_hyp(good, bad, sample);
}


double rng_triangular(double left, double mode, double right) {
  double base = right - left;
  double leftbase = mode - left;
  double ratio = leftbase / base;
  double leftprod = leftbase*base;
  double rightprod = (right - mode) * base;
  double U = sfmt_f64_3;

  if (U <= ratio)
    return left + sqrt(U * leftprod);
  else
    return right - sqrt((1. - U) * rightprod);
}


long rng_logseries(double p) {
  double q, r, U, V;
  long result;

  r = log(1. - p);

  while (1) {
    V = sfmt_f64_3;
    if (V >= p)
      return 1;
    U = sfmt_f64_3;
    q = 1. - exp(r * U);
    if (V <= q * q) {
      result = (long) floor(1 + log(V) / log(q));
      if (result < 1)
        continue;
      else
        return result;
    }
    if (V >= q) return 1;

    return 2;
  }
}


// ends here