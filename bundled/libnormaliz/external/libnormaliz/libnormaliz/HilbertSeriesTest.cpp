#include "libnormaliz/HilbertSeries.h"
#include "libnormaliz/vector_operations.h"
#include <iostream>


using namespace std;
using namespace libnormaliz;

int main() {

    vector<num_t> anum(3);
    anum[1]=1; anum[2]=1;
    vector<denom_t> adenom(2);
    adenom[0]=2; adenom[1]=3;
    
    cout << anum<<adenom;
    HilbertSeries C;
    C.add(anum,adenom);
    cout << "C: " << C;
    HilbertSeries A(anum,adenom);

    vector<num_t> bnum(5);
    bnum[0]=1; bnum[2]=1; bnum[4]=1;
    vector<denom_t> bdenom(2);
    bdenom[0]=3; bdenom[1]=3;
    
    HilbertSeries B(bnum,bdenom);

    HilbertSeries ABA = HilbertSeries();
    ABA += B;
    cout << "A: " << A;
    cout << "B: " << B;
    cout << "B: " << ABA;
    ABA += A;
    cout << "B+A: " << ABA;
    ABA += A;
    cout << "B+A+A: " << ABA << endl;

    ABA.simplify();
    cout << "Simpl: " << ABA;


    cout << endl << endl << "         *********" << endl << endl;
/*    vector<long long> p(4);
    p[3]=5; p[2]=4; p[1]=1; p[0]=3; // 5 t^3 + 4 t^2 + 1 t + 3
    cout << p;
    linear_substitution<long long>(p, 2); //transform it to q, q(t)=p(t+2)
    cout << p;
*/

    cout << "reset a HS to 0/1 and simplify" << endl;
    B.reset();
    B.simplify();
    cout << B;
    cout << "quasi-poly of that HS" << endl;
    cout << B.getHilbertQuasiPolynomial();

    cout << "creating empty HS" << endl;
    cout << HilbertSeries();

    cout << "creating HS from empty vectors" << endl;
    cout << HilbertSeries(vector<num_t>(), vector<denom_t>());

    return 0;
}
