#ifndef TTS_Deformation_h
#define TTS_Deformation_h

#include "TestCase.h"

class Deformation : public TestCase
{
public:
    enum SubCaseID {
        Case1, Case2, Case3, Case4
    };
    enum InitCondID {
        CosineHills, GaussianHills, SlottedCylinders
    };

    Deformation(SubCaseID subCaseID = Case1, InitCondID initCondID = CosineHills);
    virtual ~Deformation();

    void calcVelocityField(FlowManager &);

    void calcInitCond(TracerManager &);

private:
    SubCaseID subCaseID;
    InitCondID initCondID;
    double T; // time period
};

#endif