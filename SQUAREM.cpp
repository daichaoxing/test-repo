//TODO:
//1. Exception Handling, try&catch 1.Done
//2. additional input to each of the functions
#include <iostream>
#include <string>
#include <algorithm>
#include <Rcpp.h>
#include <RInside.h>
#include <cmath>
#include <math.h>

//#include <stdarg.h>

using namespace Rcpp;

//Global Control Variables
const int K=1;
const int method=3;//1,2,3 indicates the types of step length to be used in squarem1,squarem2, 4,5 for "rre" and "mpe" in cyclem1 and cyclem2,  standing for reduced-rank ("rre") or minimal-polynomial ("mpe") extrapolation.
// K=1 must go with method=1,2 or 3
// K>1 must go with method=4 or 5
const int mstep=4;
const int maxiter=1500;
const bool square=true;
const bool trace=false;
const double stepmin0=1;
const double stepmax0=1;
const double kr=1;
const double objfninc=1;
const double tol=1e-7;

List squarem1(NumericVector par,Function fixptfn,Function objfn=NULL);
List squarem2(NumericVector par,Function fixptfn);
List cyclem1(NumericVector par,Function fixptfn,Function objfn=NULL);
List cyclem2(NumericVector par,Function fixptfn);

Rcpp::List SQUAREM(NumericVector par,Function fixptfn,Function objfn=NULL)
{
    List sqobj;
    if (objfn==NULL) {
        if (K == 1){
            sqobj=squarem1(par, fixptfn, objfn);}
        else if(K > 1 || method>3){
            sqobj=cyclem1(par, fixptfn, objfn );}
        else{sqobj=NULL;}
    } else {
        if (K == 1){
            sqobj = squarem2(par, fixptfn);}
        else if (K > 1 || method>3){
            sqobj = cyclem2(par, fixptfn);}
        else{sqobj=NULL;}
    }
    return sqobj;
}



List squarem1(NumericVector par,Function fixptfn,Function objfn){
    SEXP lold,lnew,p,p1,p2;//R data types
    NumericVector loldcpp,lnewcpp,pcpp,p1cpp,p2cpp,pnew;
    NumericVector q1,q2,sr2,sq2,sv2,srv;
    double sr2_scalar,sq2_scalar,sv2_scalar,srv_scalar,alpha,stepmin,stepmax;
    int iter,feval,leval;
    bool conv,extrap;
    stepmin=stepmin0;
    stepmax=stepmax0;
    //pcpp=p;
    
    if(trace){std::cout<<"Squarem-1"<<std::endl;}
    if(objfn==NULL){
        std::cout<<"Squarem2 should be used if objective function is not available!"<<std::endl;
        return 0;
    }
    iter=1;p=par;lold=objfn(p);leval=1;
    loldcpp=lold;
    
    if(trace){std::cout<<"Objective fn: "<<lold<<std::endl;}
    feval=0;conv=false;
    
    while(feval<maxiter){
        //Step 1
        extrap = true;
        pcpp=p;
        try{p1=fixptfn(p);feval++;}
        catch(...){
            std::cout<<"Error in fixptfn function evaluation";
            return 1;
        }
        p1cpp=p1;
        q1=p1cpp-pcpp;
        sr2=q1*q1;
        sr2_scalar=0;
        for (int i=0;i<sr2.length();i++){sr2_scalar+=sr2[i];}
        if(sqrt(sr2_scalar)<tol){break;}
        
        //Step 2
        try{p2=fixptfn(p1);feval++;}
        catch(...){
            std::cout<<"Error in fixptfn function evaluation";
            return 1;
        }
        p2cpp=p2;
        q2=p2cpp-p1cpp;
        sq2=q2*q2;
        sq2_scalar=0;
        for (int i=0;i<q2.length();i++){sq2_scalar+=sq2[i];}
        sq2_scalar=sqrt(sq2_scalar);
        if (sq2_scalar<tol){break;}
        sv2=q2-q1;
        sv2_scalar=0;
        for (int i=0;i<sv2.length();i++){sv2_scalar+=sv2[i]*sv2[i];}
        srv_scalar=0;
        for (int i=0;i<q2.length();i++){sr2_scalar+=sv2[i]*q1[i];}
        
        //Step 3 Proposing new value
        switch (method){
            case 1: alpha= -srv_scalar/sv2_scalar;
            case 2: alpha= -sr2_scalar/srv_scalar;
            case 3: alpha= sqrt(sr2_scalar/sv2_scalar);
            default: {
                std::cout<<"Misspecification in method, when K=1, method should be either 1, 2 or 3!";
                break;}
        }
        
        alpha=std::max(stepmin,std::min(stepmax,alpha));
        pnew = pcpp + 2.0*alpha*q1 + alpha*alpha*(q2-q1);
        
        //Step 4 stabilization
        if(std::abs(alpha-1)>0.01){
            try{pnew=fixptfn(pnew);feval++;}
            catch(...){
                pnew=p2cpp;
                try{lnew=objfn(pnew);leval++;}
                catch(...){
                    lnew=lold;
                }
                if(alpha==stepmax){
                    stepmax=std::max(stepmax0,stepmax/mstep);
                }
                alpha=1;
                extrap=false;
                if(alpha==stepmax){stepmax=mstep*stepmax;}
                if(stepmin<0 && alpha==stepmin){stepmin=mstep*stepmin;}
                p=pnew;
                //DCX if(!std::isnan(lnew)){lold=lnew;}
                if(trace){std::cout<<"Objective fn: "<<lnew<<"  Extrapolation: "<<extrap<<"  Steplength: "<<alpha<<"\n"<<std::endl;}
                iter++;
                continue;
            }
            
            if (isfinite(objfninc)){
                try{lnew=objfn(pnew);leval++;}
                catch(...){
                    pnew=p2;
                    try{lnew=objfn(pnew);leval++;}
                    catch(...){
                        std::cout<<"Error in objfn function evaluation";
                        return 1;
                    }
                    if(alpha==stepmax){
                        stepmax=std::max(stepmax0,stepmax/mstep);
                    }
                    alpha=1;
                    extrap=false;
                }
            }
        }else{//same as above, when stablization is not performed.
            if (isfinite(objfninc)){
                try{lnew=objfn(pnew);leval++;}
                catch(...){
                    pnew=p2;
                    try{lnew=objfn(pnew);leval++;}
                    catch(...){
                        std::cout<<"Error in objfn function evaluation";
                        return 1;
                    }
                    if(alpha==stepmax){
                        stepmax=std::max(stepmax0,stepmax/mstep);
                    }
                    alpha=1;
                    extrap=false;
                }
            }
        }
        
        
        if(alpha==stepmax){stepmax=mstep*stepmax;}
        if(stepmin<0 && alpha==stepmin){stepmin=mstep*stepmin;}
        
        p=pnew;
        //DCX if(!std::isnan(lnew)){lold=lnew;}
        if(trace){std::cout<<"Objective fn: "<<lnew<<"  Extrapolation: "<<extrap<<"  Steplength: "<<alpha<<"\n"<<std::endl;}
        
        iter++;
    }
    
    if (feval >= maxiter){conv=false;}
    if (isfinite(objfninc)){lold=objfn(p);leval++;}
    
    return(List::create(Named("par")=p,
                        Named("value.objfn")=lold,
                        Named("iter")=iter,
                        Named("fpevals")=feval,
                        Named("objfevals")=leval,
                        Named("convergence")=conv));
}


List squarem2(NumericVector par,Function fixptfn){
    SEXP lold,lnew,p,p1,p2;//R data types
    NumericVector loldcpp,lnewcpp,pcpp,p1cpp,p2cpp,pnew;
    NumericVector q1,q2,sr2,sq2,sv2,srv;
    double sr2_scalar,sq2_scalar,sv2_scalar,srv_scalar,alpha,stepmin,stepmax;
    int iter,feval,leval;
    bool conv,extrap;
    stepmin=stepmin0;
    stepmax=stepmax0;
    pcpp=p;
    
    if(trace){std::cout<<"Squarem-1"<<std::endl;}

    iter=1;p=par;//leval=1;
    //loldcpp=lold;
    if(trace){std::cout<<"Objective fn: "<<lold<<std::endl;}
    feval=0;conv=false;
    
    while(feval<maxiter){
        //Step 1
        extrap = true;
        pcpp=p;
        try{p1=fixptfn(p);feval++;}
        catch(...){
            std::cout<<"Error in fixptfn function evaluation";
            return 1;
        }
        p1cpp=p1;
        q1=p1cpp-pcpp;
        sr2=q1*q1;
        sr2_scalar=0;
        for (int i=0;i<sr2.length();i++){sr2_scalar+=sr2[i];}
        if(sqrt(sr2_scalar)<tol){break;}
        
        //Step 2
        try{p2=fixptfn(p1);feval++;}
        catch(...){
            std::cout<<"Error in fixptfn function evaluation";
            return 1;
        }
        p2cpp=p2;
        q2=p2cpp-p1cpp;
        sq2=q2*q2;
        sq2_scalar=0;
        for (int i=0;i<q2.length();i++){sq2_scalar+=sq2[i];}
        sq2_scalar=sqrt(sq2_scalar);
        if (sq2_scalar<tol){break;}
        sv2=q2-q1;
        sv2_scalar=0;
        for (int i=0;i<sv2.length();i++){sv2_scalar+=sv2[i]*sv2[i];}
        srv_scalar=0;
        for (int i=0;i<q2.length();i++){sr2_scalar+=sv2[i]*q1[i];}
        
        //Step 3 Proposing new value
        switch (method){
            case 1: alpha= -srv_scalar/sv2_scalar;
            case 2: alpha= -sr2_scalar/srv_scalar;
            case 3: alpha= sqrt(sr2_scalar/sv2_scalar);
            default: {
                std::cout<<"Misspecification in method, when K=1, method should be either 1, 2 or 3!";
                break;}
        }
        
        alpha=std::max(stepmin,std::min(stepmax,alpha));
        pnew = pcpp + 2.0*alpha*q1 + alpha*alpha*(q2-q1);
        
        //Step 4 stabilization
        //DCX CurrentProgress
        if(std::abs(alpha-1)>0.01){
            try{pnew=fixptfn(pnew);feval++;}
            catch(...){
                pnew=p2cpp;
                if(alpha==stepmax){
                    stepmax=std::max(stepmax0,stepmax/mstep);
                }
                alpha=1;
                extrap=false;
                if(alpha==stepmax){stepmax=mstep*stepmax;}
                if(stepmin<0 && alpha==stepmin){stepmin=mstep*stepmin;}
                p=pnew;
                //DCX if(!std::isnan(lnew)){lold=lnew;}
                if(trace){std::cout<<"Objective fn: "<<lnew<<"  Extrapolation: "<<extrap<<"  Steplength: "<<alpha<<"\n"<<std::endl;}
                iter++;
                continue;
            }
            if (isfinite(objfninc)){
                try{lnew=objfn(pnew);leval++;}
                catch(...){
                    pnew=p2;
                    try{lnew=objfn(pnew);leval++;}
                    catch(...){
                        std::cout<<"Error in objfn function evaluation";
                        return 1;
                    }
                    if(alpha==stepmax){
                        stepmax=std::max(stepmax0,stepmax/mstep);
                    }
                    alpha=1;
                    extrap=false;
                }
            }
        }else{//same as above, when stablization is not performed.
            if (isfinite(objfninc)){
                try{lnew=objfn(pnew);leval++;}
                catch(...){
                    pnew=p2;
                    try{lnew=objfn(pnew);leval++;}
                    catch(...){
                        std::cout<<"Error in objfn function evaluation";
                        return 1;
                    }
                    if(alpha==stepmax){
                        stepmax=std::max(stepmax0,stepmax/mstep);
                    }
                    alpha=1;
                    extrap=false;
                }
            }
        }
    }
    
    if (feval >= maxiter){conv=false;}
    if (isfinite(objfninc)){lold=objfn(p);leval++;}
    
    return(List::create(Named("par")=p,
                        Named("value.objfn")=lold,
                        Named("iter")=iter,
                        Named("fpevals")=feval,
                        Named("objfevals")=leval,
                        Named("convergence")=conv));
}
List cyclem1(NumericVector par,Function fixptfn,Function objfn){
    List sqobj;
    return sqobj;
}
List cyclem2(NumericVector par,Function fixptfn,Function objfn){
    List sqobj;
    return sqobj;
}

 
int main(){
    std::cout<<"Hello "<<tol<<std::endl;
    double tol1=tol*10000000;
    std::cout<<"Hello "<<tol1<<std::endl;

    return 0;
}