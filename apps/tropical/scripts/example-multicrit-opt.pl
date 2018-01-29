$H = unit_matrix<TropicalNumber<Min>>(4);
$w = new Vector<TropicalNumber<Min>>([0,'inf','inf','inf']);
$v1 = new Vector<TropicalNumber<Min>>(['inf',-3,0,0]);
$H1= intersection_extremals($H,$w,$v1);
$v2 = new Vector<TropicalNumber<Min>>(['inf',0,-3,-3]);
$H2= intersection_extremals($H1,$w,$v2);
$v3 = new Vector<TropicalNumber<Min>>(['inf',-1,-2,-2]);
$H3= intersection_extremals($H2,$w,$v3);
