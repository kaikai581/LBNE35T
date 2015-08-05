// Position of hodoscope hit
TVector3 GetHitPos( int cbus, int stac, int pmt, bool hodoHigh ) {
  double hodoWidth = 60; // cm
  double hodoRaise1 = 119.4; // cm
  double hodoRaise8 = 120.0; // cm
  if ( !hodoHigh ) hodoRaise1 = 41.9; // cm
  if ( !hodoHigh ) hodoRaise8 = 42.5; // cm
  double hodoDist1 = (24.34/2+7)*2.54; // cm
  double hodoDist8 = (24.34/2+12)*2.54; // cm
  double hodoXShift1 = 0.; // cm
  double hodoXShift2 = 0.; // cm

  double stacOffsetX1[4] = { 0.0, 0.5, 0.5, 0.0 };
  double stacOffsetY1[4] = { 0.5, 0.0, 0.5, 0.0 };
  double stacOffsetX8[4] = { 0.5, 0.0, 0.0, 0.5 };
  double stacOffsetY8[4] = { 0.5, 0.5, 0.0, 0.0 };

  int i = pmt % 4;
  int j = (pmt - i) / 4;
  double y = double(i)*(1./8.) + 1./16.;
  double x = double(3-j)*(1./8.) + 1./16;
  double z = 0.;
  if ( cbus == 1 ) {
    x += stacOffsetX1[stac-1];
    y += stacOffsetY1[stac-1];
    z = 1.;
  }
  if ( cbus == 8 ) {
    x += stacOffsetX8[stac-1];
    y += stacOffsetY8[stac-1];
    z = -1.;
    x = 1. - x;
  }

  // Scale up
  x = hodoWidth * x;
  y = hodoWidth * y;

  x = x - 0.5*hodoWidth;
  if ( cbus == 1 ) y += hodoRaise1;
  if ( cbus == 8 ) y += hodoRaise8;

  if ( cbus == 1 ) x += hodoXShift1;
  if ( cbus == 8 ) x += hodoXShift2;

  if ( cbus == 1 ) z = hodoDist1 * z;
  if ( cbus == 8 ) z = hodoDist8 * z;

  return TVector3(x,y,z);
}
