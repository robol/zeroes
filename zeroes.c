#include <stdio.h>
#include <math.h>
#include <time.h>

#ifdef USE_MPSOLVE
#include <mps/mps.h>
#else
#include <gsl/gsl_poly.h>
#include <gsl/gsl_errno.h>
#endif

int main (int argc, char* argv[])
{
  /* Parse the command line arguments. */
  const char* usage = "Usage: zeroes <xmin> <xmax> <ymin> <ymax> <xres> <coeff> ... <coeff> - <degree> ... <degree>\n";

  if (argc < 8) {
    fprintf (stderr, "%s", usage);
    exit(EXIT_FAILURE);
  }

  /* The window in the plane. */
  const double xmin = atof(argv[1]);
  const double xmax = atof(argv[2]);
  const double ymin = atof(argv[3]);
  const double ymax = atof(argv[4]);

  /* Image size, width is given, height is computed. */
  const int xres = atoi(argv[5]);
  const int yres = (int)((xres * (ymax-ymin)) / (xmax-xmin));

  /* The coefficients */
  int ci_max = 0 ;
  while (6 + ci_max < argc && (argv[6+ci_max][0] != '-' || argv[6+ci_max][1] != 0)) { ci_max++; }
  if (ci_max == 0) {
    fprintf (stderr, "%s\nError: specify at least one coefficient.", usage) ;
    exit(EXIT_FAILURE);
  }
  double coeff[ci_max];
  for (int ci = 0; ci < ci_max; ci++) { coeff[ci] = atof(argv[6+ci]); }

  /* The degrees */
  const int di_max = argc - 7 - ci_max;
  if (di_max <= 0) {
    fprintf (stderr, "%s\nError: specify at least one degree.", usage) ;
    exit(EXIT_FAILURE);
  }
  int degree[di_max];
  for (int di = 0; di < di_max; di++) { degree[di] = atoi(argv[7+ci_max+di]); }
  
  /* Allocate the image and initialize it */
  unsigned int *image = (unsigned int *)calloc(sizeof(unsigned int), yres * xres) ;
  const unsigned int count_bound = 0xffffffff;
  unsigned int max_count = 0 ;

#ifndef USE_MPSOLVE
  /* Turn off the GSL error handler that aborts on error (some polynomials have tricky zeroes) */
  gsl_set_error_handler_off ();
#endif

  /* We're going to skip zeroes whose imaginary part is almost zero. */
  const double epsilon = 1.0e-20 ;

  time_t start = time(NULL);

  /* Run through degrees and compute the zeroes for each one. */
  for (int di = 0; di < di_max; di++) {
    const int d = degree[di]; /* The degree we're working on. */
    time_t now = time(NULL);
    fprintf (stderr, "Degree %d (%.2lf seconds)\n", d, difftime(now, start));
    /* Initialize the polynomial. */
    double poly[d+1];

    for (int j = 0; j <= d; j++) { poly[j] = coeff[0]; } 
    /* Initialize the counters. */
    int counter[d+1];
    for (int j = 0; j <= d; j++) { counter[j] = 0; }
    int j = 0;

#ifndef USE_MPSOLVE
    /* Allocate the workspace. */
    gsl_poly_complex_workspace *w = gsl_poly_complex_workspace_alloc(d+1);
#endif

    do {

      /* Compute the zeroes if head coefficient is non-zero. */
      if (poly[d] > epsilon || poly[d] < -epsilon) {

        double z[2*d]; /* the zeroes are stored here */
#ifdef USE_MPSOLVE
	mps_context * ctx = mps_context_new ();
	mps_monomial_poly * mp = mps_monomial_poly_new (ctx, d);
	cplx_t * zz = NULL;
	
	for (int j = 0; j <= d; j++) { 
	  mps_monomial_poly_set_coefficient_int (ctx, mp, j, poly[j], 0);
	}
	
	mps_context_set_input_poly (ctx, MPS_POLYNOMIAL (mp));
	mps_context_set_output_prec (ctx, 8);
	mps_mpsolve (ctx);

	int status = mps_context_has_errors (ctx);

	if (! status)
	  {
	    mps_context_get_roots_d (ctx, &zz, NULL);

	    for (int i = 0; i < d; i++)
	      {
		z[2*i] = cplx_Re (zz[i]);
		z[2*i+1] = cplx_Im (zz[i]);
	      }

	    free (zz);
	  }

	/* Deallocate workspace. */
	mps_monomial_poly_free (ctx, MPS_POLYNOMIAL (mp));
	mps_context_free (ctx);	
#else
        int status = gsl_poly_complex_solve (poly, d+1, w, z);
#endif



	if (status == 0) {
	  /* Draw zeroes, skipping the real ones that are away from the origin */
	  for (int i=0; i < d; i++) {
	    if (-epsilon < z[2*i+1] && z[2*i+1] < epsilon && (z[2*i] < -0.5 || z[2*i] > 0.5)) {
	      continue;
	    }
	    int x = (int)((xres * (z[2*i] - xmin)) / (xmax - xmin));
	    int y = yres - (int)((yres * (z[2*i+1] - ymin)) / (ymax - ymin));
	    if (0 <= x && x < xres && 0 <= y && y < yres && image[xres * y + x] < count_bound) {
	      int c = ++image[xres * y + x];
	      if (max_count < c) { max_count = c; }
	    }
	  }
	}
      }
      /* calculate the next polynomial */
      for (j = 0; j <= d && counter[j] == ci_max-1; j++) { counter[j] = 0; poly[j] = coeff[0]; }
      if (j <= d) { counter[j]++; poly[j] = coeff[counter[j]]; }
    } while (j <= d);
  }

  /* Output the computed result. */
  /* ASCII header to the file*/
  printf("P6\n# Zeroes, xmin=%lf, xmax=%lf, ymin=%lf, ymax=%lf, max_count=%d, coeffs=[", xmin, xmax, ymin, ymax, max_count);
  for (int ci=0; ci < ci_max; ci++) { printf("%lf%s", coeff[ci], (ci < ci_max-1 ? ", " : "],")); }
  printf("degrees = [");
  for (int di=0; di < di_max; di++) { printf("%d%s", degree[di], (di < di_max-1 ? ", " : "],")); }
  printf("\n");
  printf("%d\n%d\n%d\n", xres, yres, 0xffff);
  /* Image */
  double r = 65535.0 / log((double)max_count) ;
  for (int y = 0; y < yres; y++) {
    for (int x = 0; x < xres; x++) {
        unsigned char color[6];
        unsigned int k = (unsigned int)(r * log((float)image[xres * y + x])) ;
        color[0] = k >> 8;
        color[1] = k & 255;
        color[2] = k >> 8;
        color[3] = k & 255;
        color[4] = k >> 8;
        color[5] = k & 255;
        fwrite(color, 6, 1, stdout);
    }
  }
  free(image);
  return(0);
}
