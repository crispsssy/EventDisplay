#ifndef _CDCHIT_HXX_
#define _CDCHIT_HXX_

#include <iostream>
#include <vector>

class CDCHit
{
public:
	CDCHit() {}
	CDCHit(int channel){ fChannelID = channel; }
	CDCHit(int channel, double z){ fChannelID = channel, fZ = z; }
	~CDCHit() {}

	void InsertADC(short adc){ fADC.push_back(adc); }
	void InsertTDC(int tdc){ fTDC.push_back(tdc); }
	void InsertDriftTime(double driftTime){ fDriftTime.push_back(driftTime); }
	void SetDOCA(double DOCA){ fDOCA = DOCA; }
	void SetZ(double z){ fZ = z; }
	bool IncreaseTDCIndex();
	void InsertDriftDistance(double dis){ fDriftDistance.push_back(dis); }
	void SetADCSumWithUnderShoot(int sum){ fADCSumWithUndershoot = sum; }
	void SetADCSumWithoutUnderShoot(int sum){ fADCSumWithoutUndershoot = sum; }
	std::vector<short> const GetADC() const { return fADC; }
	std::vector<int> const GetTDC() const { return fTDC; }
	int const GetChannelID() const { return fChannelID; }
	double const GetDriftTime(int index) const { return fDriftTime.at(index); }
	std::vector<double> const GetDriftTime() const { return fDriftTime; }
	double const GetDOCA() const { return fDOCA; }
	double const GetZ() const { return fZ; }
	double const GetDriftDistance(int index) const { return fDriftDistance.at(index); }
	std::vector<double> const GetDriftDistance() const { return fDriftDistance; }
	int const GetADCSumWithUnderShoot() const { return fADCSumWithUndershoot; }
	int const GetADCSumWithoutUnderShoot() const { return fADCSumWithoutUndershoot; }

	void ls() const;

private:
	int fChannelID = -999;
	std::vector<short> fADC;
	std::vector<int> fTDC;
	std::vector<double> fDriftTime;
	std::vector<double> fDriftDistance;
	double fDOCA = 0.;
	double fZ = 0.;
	unsigned int fUseTDC = 0; //determine which TDC will be used for fitting
	int fADCSumWithUndershoot = 0;
	int fADCSumWithoutUndershoot = 0;
};

class CDCHitContainer : public std::vector<CDCHit*>
{
public:

	CDCHitContainer() {}
	~CDCHitContainer() {}
	void ls() const; 

};

#endif
