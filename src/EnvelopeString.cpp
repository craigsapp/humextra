//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Sun Nov 23 18:46:35 GMT-0800 1997
// Last Modified: Mon Apr 13 11:06:15 PDT 1998
// Filename:      ...sig/maint/code/base/EnvelopeString/EnvelopeString.cpp
// Web Address:   http://sig.sapp.org/src/sig/EnvelopeString.cpp
// Documentation: http://sig.sapp.org/doc/classes/EnvelopeString
// Syntax:        C++
//

#include "EnvelopeString.h"

#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

using namespace std;

#define MAX_ENVELOPE_POINT_DIMENSION (100)


//////////////////////////////
//
// EnvelopeString::EnvelopeString --
//

EnvelopeString::EnvelopeString(void) {
	envelope = NULL;
	normalization = 1.0;
	dimension = 0;
	m_points.resize(0);
	absoluteType = 'm';
	m_pointInterp.resize(0);
	m_pointParameter.resize(0);
}

EnvelopeString::EnvelopeString(const char* aString) {
	envelope = NULL;
	normalization = 1.0;
	dimension = 0;
	m_points.resize(0);
	absoluteType = 'm';
	m_pointInterp.resize(0);
	m_pointParameter.resize(0);

	setEnvelope(aString);
}



//////////////////////////////
//
// EnvelopeString::~EnvelopeString --
//

EnvelopeString::~EnvelopeString() {
	if (envelope != NULL) {
		delete [] envelope;
	}
   m_points.resize(0);
}



//////////////////////////////
//
// EnvelopeString::getDimension --
//

int EnvelopeString::getDimension(void) const {
	return dimension;
}



//////////////////////////////
//
// EnvelopeString::getEnvelope --
//

const char* EnvelopeString::getEnvelope(void) {
	if (!modified) {
		return envelope;
	} else {
		makeEnvelope();
		return envelope;
	}
}



//////////////////////////////
//
// EnvelopeString::getLength --
//

double EnvelopeString::getLength(void) const {
	return normalization;
}



//////////////////////////////
//
// EnvelopeString::getNumPoints --
//

int EnvelopeString::getNumPoints(void) const {
   if (!m_points.empty()) {
	   return (int)m_points[0].size();
	} else {
		return 0;
	}
}



//////////////////////////////
//
// EnvelopeString::getStickIndex -- return -1 if no stick point
//

int EnvelopeString::getStickIndex(void) const {
	return stickIndex;
}



//////////////////////////////
//
// EnvelopeString::getValue --
//

double EnvelopeString::getValue(int a, int b) {
	return m_points.at(b).at(a);
}



//////////////////////////////
//
// EnvelopeString::print --
//

void EnvelopeString::print(void) {
   if (m_points.empty()) {
      return;
   }

	cout << "   Envelope Time Length = " << getLength() << endl;
	cout << "   Default interpolation = " << defaultInterpolation << endl;
	cout << "   Absolute Type = " << absoluteType << endl;
	cout << "   Stick index = " << stickIndex << endl;
	cout << "   Number of m_points = " << m_points[0].size() << endl;
	for (int i=0; i<(int)m_points[0].size(); i++) {
		for (int j=0; j<getDimension(); j++) {
			cout.width(10);
			cout << m_points.at(j).at(i) << "  ";
		}

		if (m_pointInterp[i] != '\0') {
			cout.width(3);
			cout << m_pointInterp[i] << "  ";
			cout << m_pointParameter[i];
		} else {
			cout.width(3);
			cout << defaultInterpolation << "  ";
			cout << defaultParameter;
		}

		cout << endl;
	}
}



//////////////////////////////
//
// EnvelopeString::removeStickPoint --
//

void EnvelopeString::removeStickPoint(void) {
	for (int i=0; i<getNumPoints()-1; i++) {
		if (m_points.at(0).at(i) < 0) {
			m_points.at(0).at(i) = 1.0 + m_points.at(0).at(i);
		}
	}
	stickIndex = -1;
	modified = 1;
}



//////////////////////////////
//
// EnvelopeString::setEnvelope --
//	default values: aDimension = -1, stringType = UNKNOWN_ENV
//

void EnvelopeString::setEnvelope(const char *aString, int aDimension,
		int stringType) {
	if (aString == NULL) {
		cerr << "Error: Input to setEnvelope has no length" << endl;
		exit(1);
	}

	// check to see if there is only one number, if so then
	// create a constant envelope
	char *realEnvelope = new char[strlen(aString) + 100];
	char *tmpstring = new char[strlen(aString) + 1];
	strcpy(tmpstring, aString);
	strtok(tmpstring, " \n\t[](){},|");
	if (strtok(NULL, " \n\t[](){};,|") == NULL) {
		delete [] tmpstring;
		strcpy(realEnvelope, "0 ");
		strcat(realEnvelope, aString);
		strcat(realEnvelope, " 1 ");
		strcat(realEnvelope, aString);
	} else {
		delete [] tmpstring;
		strcpy(realEnvelope, aString);
	}


	int stype = UNKNOWN_ENV;
	if (stringType == UNKNOWN_ENV) {
		stype = determineType(realEnvelope);
	}
	if (stype == UNKNOWN_ENV) {
		cerr << "Error: unknown type of envelope: " << realEnvelope << endl;
		exit(1);
	}

	switch (stype) {
		case CLM_ENV:
			processCLMenv(realEnvelope, aDimension);
			break;
		case LISP_ENV:
			processLISPenv(realEnvelope);
			break;
		case MUSICKIT_ENV:
			processMKenv(realEnvelope);
			break;
		case MATHEMATICA_ENV:
			processMMAenv(realEnvelope);
			break;
		case PLAIN_ENV:
			processPLAINenv(realEnvelope, aDimension);
			break;
		case SIG_ENV:
			processSIGenv(realEnvelope);
			break;
		default:
			cout << "Error: unknown type of envelope." << endl;
			exit(1);
	}

	adjustForStick();

	// Successful read of envelope, now store new envelope string
	if (envelope != NULL) {
		delete [] envelope;
	}
	envelope = new char[strlen(realEnvelope)+1];
	strcpy(envelope, realEnvelope);
	modified = 0;
	delete [] realEnvelope;
}



//////////////////////////////
//
// EnvelopeString::setInterpolation -- sets the global default
//	interpolation between points.
//

void EnvelopeString::setInterpolation(char aType) {
	validateInterpolation(aType);
	defaultInterpolation = aType;
	modified = 1;
}



//////////////////////////////
//
// EnvelopeString::setLength --
//

void EnvelopeString::setLength(double aLength) {
	if (aLength <= 0.0) {
		cerr << "Error: cannot have a non-positive envelope duration." << endl;
		exit(1);
	}
	normalization = aLength;
	modified = 1;
}



//////////////////////////////
//
// EnvelopeString::setStickIndex --
//

void EnvelopeString::setStickIndex(int anIndex) {
	if (anIndex < 1 || anIndex > getNumPoints() - 1) {
		cerr << "Error: invalid index for stick point: " << anIndex << endl;
		exit(1);
	}
	if (anIndex == stickIndex) return;

	for (int i=0; i<getNumPoints(); i++) {
		if (m_points.at(0).at(i) < 0) {
			m_points.at(0).at(i) = m_points.at(0).at(i) + 1.0;
		}
		if (i >= anIndex) {
			m_points.at(0).at(i) = m_points.at(0).at(i) - 1.0;
		}
	}

	stickIndex = anIndex;
	modified = 1;
}



//////////////////////////////
//
// setStickSamples
//

void EnvelopeString::setStickSamples(void) {
	absoluteType = 'm';
	modified = 1;
}



//////////////////////////////
//
// setStickSeconds
//

void EnvelopeString::setStickSeconds(void) {
	absoluteType = 't';
	modified = 1;
}



//////////////////////////////
//
// EnvelopeString::setSyntax --
//

void EnvelopeString::setSyntax(int syntaxType) {
	switch (syntaxType) {
		case CLM_ENV:
		case LISP_ENV:
		case MUSICKIT_ENV:
		case MATHEMATICA_ENV:
		case PLAIN_ENV:
		case SIG_ENV:
			outputType = syntaxType;
			break;
		default:
			cerr << "Error: unknown type of syntax." << endl;

	}
	modified = 1;
}



//////////////////////////////
//
// EnvelopeString::setSyntaxCLM --
//

void EnvelopeString::setSyntaxCLM(void) {
	setSyntax(CLM_ENV);
}



//////////////////////////////
//
// EnvelopeString::setSyntaxLISP --
//

void EnvelopeString::setSyntaxLISP(void) {
	setSyntax(LISP_ENV);
}



//////////////////////////////
//
// EnvelopeString::setSyntaxMK --
//

void EnvelopeString::setSyntaxMK(void) {
	setSyntax(MUSICKIT_ENV);
}



//////////////////////////////
//
// EnvelopeString::setSyntaxMMA --
//

void EnvelopeString::setSyntaxMMA(void) {
	setSyntax(MMA_ENV);
}



//////////////////////////////
//
// EnvelopeString::setSyntaxPLAIN --
//

void EnvelopeString::setSyntaxPLAIN(void) {
	setSyntax(PLAIN_ENV);
}



//////////////////////////////
//
// EnvelopeString::setSyntaxSIG --
//

void EnvelopeString::setSyntaxSIG(void) {
	setSyntax(SIG_ENV);
}



//////////////////////////////
//
// operator<<
//

ostream& operator<<(ostream& out, EnvelopeString& aString) {
	out << aString.getEnvelope();

	return out;
}



///////////////////////////////////////////////////////////////////////////
//
// Private functions
//


//////////////////////////////
//
// EnvelopeString::determineType --
//

int EnvelopeString::determineType(const char* aString) {
	int length = strlen(aString);
	int index;

	// if the first non-space character is a '[' then a MusicKit envelope
	for (index=0; index<length; index++) {
		if (!std::isspace(aString[index])) {
			if (aString[index] == '[') return MUSICKIT_ENV;
			else break;
		}
	}

	// if the first non-space character is a '{' then a Mathematica envelope
	for (index=0; index<length; index++) {
		if (!std::isspace(aString[index])) {
			if (aString[index] == '{') return MMA_ENV;
			else break;
		}
	}


	// if the envelope contains a ';' then it is a sig envelope
	for  (index=0; index<length; index++) {
		if (aString[index] == ';') {
			return SIG_ENV;
		}
	}


	// Must be a LISP envelope or close relative:
	index = 0;
	int openCount, closeCount;
	openCount = closeCount = 0;
	for (index=0; index<length; index++) {
		if (aString[index] == '(') openCount++;
		else if (aString[index] == ')') closeCount++;
	}

	if (closeCount != openCount) {
		// unmatched parentheses
		return UNKNOWN_ENV;
	}
	if (openCount == 0) {
		// plain envelope
		return PLAIN_ENV;
	} else if (openCount == 1) {
		// parenthesis envelope
		return CLM_ENV;
	} else {
		// LISP envelope
		return LISP_ENV;
	}

}



//////////////////////////////
//
// EnvelopeString::extractNumber -- extract a number from an
//    envelope and move the index to point to the character
//    following the extracted number
//

double EnvelopeString::extractNumber(const char* aString, int& index) {
	double output = -1;

	// skip to the number to extract
	while (aString[index] != '\0') {
		if (std::isdigit(aString[index]) ||
			   aString[index] == '+' ||
			   aString[index] == '-' ||
			   aString[index] == '.' ||
			   aString[index] == 'e' ||
			   aString[index] == 'E'    ) {
			break;
		}
		index++;
	}

	output = atof(&aString[index]);

	// skip number  just read
	while (aString[index] != '\0') {
		if (std::isdigit(aString[index]) ||
			   aString[index] == '+' ||
			   aString[index] == '-' ||
			   aString[index] == '.' ||
			   aString[index] == 'e' ||
			   aString[index] == 'E'    ) {
			index++;
		} else {
			break;
		}
	}

	// skip any white space after number
	while (aString[index] != '\0') {
		if (std::isspace(aString[index])) {
			index++;
		} else {
			break;
		}
	}

	return output;
}



//////////////////////////////
//
// EnvelopeString::makeEnvelope --
//

void EnvelopeString::makeEnvelope(void) {
	switch (outputType) {
		case CLM_ENV:
			makeCLMenv();
			break;
		case LISP_ENV:
			makeLISPenv();
			break;
		case MUSICKIT_ENV:
			makeMKenv();
			break;
		case MMA_ENV:
			makeMMAenv();
			break;
		case PLAIN_ENV:
			makePLAINenv();
			break;
		case SIG_ENV:
			makeSIGenv();
			break;
		default:
			cerr << "Error: Unknown type of envelope syntax." << endl;
			exit(1);
	}

	modified = 0;
};


//////////////////////////////
//
// EnvelopeString::makeCLMenv -- called only by makeEnvelope
//

void EnvelopeString::makeCLMenv(void) {
	stringstream newString;
	int i, j;

	newString << "(";
	for (i=0; i<getNumPoints(); i++) {
		// ignore stick point
		if (i != 0) {
			newString << " ";
		}
		if (m_points.at(0).at(i) < 0.0) {
			newString << (1.0 + m_points.at(0).at(i)) * getLength();
		} else {
			newString << m_points.at(0).at(i) * getLength();
		}
		for (j=1; j<getDimension(); j++) {
			newString << " ";
			newString << m_points.at(j).at(i);
		}
	}
	newString << ")";

	newString << ends;
	int length = strlen(newString.str().c_str());
	if (envelope != NULL) {
		delete [] envelope;
	}
	envelope = new char[length + 1];
	strcpy(envelope, newString.str().c_str());

	modified = 0;
}



//////////////////////////////
//
// EnvelopeString::makeLISPenv -- called only by makeEnvelope
//

void EnvelopeString::makeLISPenv(void) {
	stringstream newString;

	newString << "(";
	for (int i=0; i<(int)m_points.at(0).size(); i++) {
		// ignore stick point
		newString << "(";
		if (m_points[0][i] < 0.0) {
			newString << (1.0 + m_points[0][i]) * getLength();
		} else {
			newString << m_points[0][i] * getLength();
		}
		for (int j=1; j<getDimension(); j++) {
			newString << " ";
			newString << m_points[j][i];
		}
		newString << ")";
	}
	newString << ")";

	newString << ends;
	int length = strlen(newString.str().c_str());
	if (envelope != NULL) {
		delete [] envelope;
	}
	envelope = new char[length + 1];
	strcpy(envelope, newString.str().c_str());

	modified = 0;
}



//////////////////////////////
//
// EnvelopeString::makeMKenv -- called only by makeEnvelope
//

void EnvelopeString::makeMKenv(void) {
	stringstream newString;

	newString << "[";
	for (int i=0; i<(int)m_points.at(0).size(); i++) {
		if (i == stickIndex) {
			newString << "|";
		}
		newString << "(";
		if (m_points[0][i] < 0.0) {
			newString << (1.0 + m_points[0][i]) * getLength();
		} else {
			newString << m_points[0][i] * getLength();
		}
		for (int j=1; j<getDimension(); j++) {
			newString << ", ";
			newString << m_points[j][i];
		}
		newString << ")";
	}
	newString << "]";


	newString << ends;
	int length = strlen(newString.str().c_str());
	if (envelope != NULL) {
		delete [] envelope;
	}
	envelope = new char[length + 1];
	strcpy(envelope, newString.str().c_str());

	modified = 0;
}



//////////////////////////////
//
// EnvelopeString::makeMMAenv -- called only by makeEnvelope
//

void EnvelopeString::makeMMAenv(void) {
	stringstream newString;
	int i, j;

	newString << "{";
	for (i=0; i<getNumPoints(); i++) {
		// ignore stick point
		newString << "{";
		if (m_points.at(0)[i] < 0.0) {
			newString << (1.0 + m_points[0][i]) * getLength();
		} else {
			newString << m_points[0][i] * getLength();
		}
		for (j=1; j<getDimension(); j++) {
			newString << ", ";
			newString << m_points[j][i];
		}
		if (i != getNumPoints()-1) {
			newString << "},";
		} else {
			newString << "}";
		}
	}
	newString << "}";

	newString << ends;
	int length = strlen(newString.str().c_str());
	if (envelope != NULL) {
		delete [] envelope;
	}
	envelope = new char[length + 1];
	strcpy(envelope, newString.str().c_str());

	modified = 0;
}



//////////////////////////////
//
// EnvelopeString::makePLAINenv -- called only by makeEnvelope
//

void EnvelopeString::makePLAINenv(void) {
	stringstream newString;
	int i, j;

	for (i=0; i<getNumPoints(); i++) {
		// ignore stick point
		if (m_points.at(0).at(i) < 0.0) {
			newString << (1.0 + m_points[0][i]) * getLength();
		} else {
			newString << m_points[0][i] * getLength();
		}
		for (j=1; j<getDimension(); j++) {
			newString << ", ";
			newString << m_points[j][i];
		}
		if (i != getNumPoints()-1) {
			newString << ", ";
		}
	}

	newString << ends;
	int length = strlen(newString.str().c_str());
	if (envelope != NULL) {
		delete [] envelope;
	}
	envelope = new char[length + 1];
	strcpy(envelope, newString.str().c_str());

	modified = 0;
}



//////////////////////////////
//
// EnvelopeString::makeSIGenv -- called only by makeEnvelope
//

void EnvelopeString::makeSIGenv(void) {
	stringstream newString;

	if (std::toupper(defaultInterpolation) != 'L') {
		newString << defaultInterpolation;
	}
	// put the default parameter here later.

	newString << "(";
	for (int i=0; i<(int)m_points.at(0).size(); i++) {
		if (i == stickIndex) {
			newString << " s;";
		}
		if (i != 0) {
			newString << " ";
		}
		if (m_points[0][i] < 0.0) {
			newString << (1.0 + m_points[0][i]) * getLength();
		} else {
			newString << m_points[0][i] * getLength();
		}
		for (int j=1; j<getDimension(); j++) {
			newString << " ";
			newString << m_points[j][i];
		}

		// deal with interpolation
		if (m_pointInterp[i] != '\0') {
			newString << " " << m_pointInterp[i];
		}
		// deal with default parameter here later.

		if (i != (int)m_points[0].size() - 1) {
			newString << ";";
		}
	}
	newString << ")";

	newString << ends;
	int length = strlen(newString.str().c_str());
	if (envelope != NULL) {
		delete [] envelope;
	}
	envelope = new char[length + 1];
	strcpy(envelope, newString.str().c_str());

	modified = 0;
}



//////////////////////////////
//
// processCLMenv -- reads a CLM envelope into internal data format
//	default value: dimension = -1
//

void EnvelopeString::processCLMenv(const char* aString, int aDimension) {

	int length = strlen(aString);
	int i, j, index;
	stickIndex = -1;

	defaultInterpolation = 'L';
	defaultParameter = -9999;
	absoluteType = 't';

	vector<double> tempData;

	index = 0;
	skipSpace(aString, index);

	if (aString[index] != '(') {
		cerr << "Error at beginning of string: " << aString << endl;
		exit(1);
	}
	index++;

	i = 0;
	while (aString[index] != ')' && index < length) {
		tempData.push_back(extractNumber(aString, index));
		skipSpace(aString, index);
	}
	if (aString[index] != ')') {
		cerr << "Error at end of envelope at character " << index + 1
			  << " in: " << aString << endl;
		exit(1);
	}

	if (aDimension == -1) {
		dimension = 2;
	} else {
		dimension = aDimension;
	}
	if (getDimension() < 2) {
		cerr << "Error: not enough dimension to envelope points: "
			  << aString << endl;
		exit(1);
	}

	if (tempData.size() % getDimension() != 0) {
		cerr << "Error: CLM envelope must have right amount of data for "
			  << "dimension : " << aString << endl;
		exit(1);
	}
	int pointCount = (int)tempData.size() / getDimension();
	setLength(tempData[(int)tempData.size()-getDimension()]);

   m_points.resize(0);
   m_points.resize(getDimension());
	for (i=0; i<(int)m_points.size(); i++) {
		m_points[i].reserve(pointCount);
		m_points[i].resize(0);
	}
	m_pointInterp.reserve(pointCount);
	m_pointInterp.resize(0);
	m_pointParameter.reserve(pointCount);
	m_pointParameter.resize(0);

	for (i=0; i<pointCount; i++) {
		for (j=0; j<getDimension(); j++) {
			m_points.at(j).at(i) = tempData.at(i * getDimension() + j);
		}
		m_points[0][i] /= getLength();
		m_pointInterp[i] = '\0';
		m_pointParameter[i] = -9999;
	}

}



//////////////////////////////
//
// processLISPenv -- reads a LISP envelope into internal data format
//

void EnvelopeString::processLISPenv(const char* aString) {
	int length = strlen(aString);
	int i, j, index;
	stickIndex = -1;

	defaultInterpolation = 'L';
	defaultParameter = -9999;

	// count the number of points in the envelope by counting the
	// number of open parenthesis and close parenthesis:
	int pointCount;
	int openP, closeP;
	openP = closeP = 0;
	for (i=0; i < length; i++) {
		if (aString[i] == '(') {
			openP++;
		} else if (aString[i] == ')') {
			closeP++;
		}
	}
	if (openP != closeP) {
		cerr << "Error: parenthesis do not match: " << aString << endl;
		exit(1);
	}
	pointCount = openP-1;  // extra outer parentheses
	if (pointCount < 1) {
		cerr << "Error: no points in envelope: " << aString << endl;
		exit(1);
	}

	// find the first coordinate of the last point in the envelope
	// to use in normalizing the time element of the envelope:
	index = length-1;
	while (aString[index] != '(' && index >= 0) {
		index--;
	}
	setLength(extractNumber(aString, index));

	// no interpolation indicators allowed.
	// no interpolation parameters allowed.

	index = 0;
	skipSpace(aString, index);

	// must be at start of envelope
	if (aString[index] != '(') {
		cerr << "Error at start of envelope: " << aString << endl;
		exit(1);
	}
	index++;
	skipSpace(aString, index);
	if (aString[index] != '(') {
		cerr << "Error at beginning of fist data point in envelope: "
			  << aString << endl;
		exit(1);
	}
	index++;


	// determine the dimension of an envelope point by checking
	// the number of elements in the first envelope point:
	dimension = 1;
	double testNumbers[MAX_ENVELOPE_POINT_DIMENSION];
	testNumbers[getDimension()-1] = extractNumber(aString, index);
	if (testNumbers[getDimension()-1] != 0.0) {
		cerr << "Error: first element in an envelope must be zero: "
			  << testNumbers[getDimension()-1] << " in " << aString << endl;
		exit(1);
	}
	skipSpace(aString, index);
	while (aString[index] != ')' && aString[index] != '\0') {
		dimension++;
		if (getDimension() >= MAX_ENVELOPE_POINT_DIMENSION) {
			cerr << "Error: too many elements in first point of envelope: "
			     << aString << endl;
			exit(1);
		}
		testNumbers[getDimension()-1] = extractNumber(aString, index);
		skipSpace(aString, index, "\n \t,");
	}


	if (getDimension() <= 1) {
		cerr << "Error: not enough data for first envelope point: "
			  << aString << endl;
		exit(1);
	}


	// process the rest of the envelope points
	m_points.resize(getDimension());

	for (int i=0; i<(int)m_points.size(); i++) {
		m_points[i].reserve(pointCount);
		m_points[i].resize(0);
		m_points[i].push_back(testNumbers[i]);
	}
	m_pointInterp.reserve(pointCount);
	m_pointInterp.resize(0);
	m_pointParameter.reserve(pointCount);
	m_pointParameter.resize(0);

	m_pointInterp[0] = '\0';
	m_pointParameter[0] = -9999;

	// no interpolation data allowed in envelope

	skipSpace(aString, index);
	if (aString[index] != ')') {
		cerr << "Error: unexpected continuation of data point at character "
			  << index + 1 << " in envelope: " << aString << endl;
		exit(1);
	}
	index++;


	// fill in the reset of the envelope point data
	for (i=1; i<pointCount; i++) {
		skipSpace(aString, index);
		if (aString[index] == '|') {
			if (stickIndex != -1) {
				cerr << "Error: only one stick point allowd in envelope: "
			        << aString << endl;
				exit(1);
			} else {
				stickIndex = i;
			}
			index++;
			skipSpace(aString, index);
		}
		if (aString[index] != '(') {
			cerr << "Error: at character " << index + 1 << " in envelope: "
			     << aString << endl;
			exit(1);
		}
		index++;

		m_points[0][i] = extractNumber(aString, index);
		m_points[0][i] /= getLength();
		if (m_points[0][i] < m_points[0][i-1]) {
			cerr << "Error: time values in envelope must always increase." << endl;
			cerr << "Check the time values: " << m_points[0][i-1] * getLength()
			      << " and " << m_points[0][i-1] * getLength()
			      << " around character " << index + 1 << " in envelope: "
			      << aString << endl;
			exit(1);
		}

		m_pointInterp[i] = '\0';
		m_pointParameter[i] = -9999;

		// no interpolation data allowed in envelope

		// get envelope point data
		j = 1;
		skipSpace(aString, index);
		if (aString[index] != ')') {
			for (j=1; j<getDimension(); j++) {
				m_points[j][i] = extractNumber(aString, index);
				skipSpace(aString, index);
				// no interpolation data allowed
			}
		}

		// sticky data is not allowed

		if (j < getDimension()) {
			cerr << "Error: not enough point data at character "
			     << index + 1 << " in envelope: " << aString << endl;
			exit(1);
		}

		skipSpace(aString, index);
		if (aString[index] != ')') {
			cerr << "Error: unexpected continuation of data point at character "
			     << index + 1 << " in envelope: " << aString << endl;
			exit(1);
		}
		index++;
		skipSpace(aString, index);

	} // end of for i loop

	if (aString[index] != ')') {
		cerr << "Error at end of envelope at character " << index + 1
			  << " in: " << aString << endl;
		exit(1);
	}

}



//////////////////////////////
//
// processMKenv
//

void EnvelopeString::processMKenv(const char* aString) {
	int length = strlen(aString);
	int i, j, index;
	stickIndex = -1;

	defaultInterpolation = 'L';
	absoluteType = 't';
	defaultParameter = -9999;

	// count the number of points in the envelope by counting the
	// number of open parenthesis and close parenthesis:
	int pointCount;
	int openP, closeP;
	openP = closeP = 0;
	for (i=0; i < length; i++) {
		if (aString[i] == '(') {
			openP++;
		} else if (aString[i] == ')') {
			closeP++;
		}
	}
	if (openP != closeP) {
		cerr << "Error: parenthesis do not match: " << aString << endl;
		exit(1);
	}
	pointCount = openP;
	if (pointCount < 1) {
		cerr << "Error: no points in envelope: " << aString << endl;
		exit(1);
	}


	// find the first coordinate of the last point in the envelope
	// to use in normalizing the time element of the envelope:
	index = length-1;
	while (aString[index] != '(' && index >= 0) {
		index--;
	}
	setLength(extractNumber(aString, index));

	// no interpolation indicators allowed.
	// no interpolation parameters allowed.

	index = 0;
	skipSpace(aString, index);

	// must be at start of envelope
	if (aString[index] != '[') {
		cerr << "Error at start of envelope: " << aString << endl;
		exit(1);
	}
	index++;
	skipSpace(aString, index);
	if (aString[index] != '(') {
		cerr << "Error at beginning of fist data point in envelope: "
			  << aString << endl;
		exit(1);
	}
	index++;


	// determine the dimension of an envelope point by checking
	// the number of elements in the first envelope point:
	dimension = 1;
	double testNumbers[MAX_ENVELOPE_POINT_DIMENSION];
	testNumbers[getDimension()-1] = extractNumber(aString, index);
	if (testNumbers[getDimension()-1] != 0.0) {
		cerr << "Error: first element in an envelope must be zero: "
			  << testNumbers[getDimension()-1] << " in " << aString << endl;
		exit(1);
	}
	skipSpace(aString, index);
	while (aString[index] != ')' && aString[index] != '\0') {
		dimension++;
		if (getDimension() >= MAX_ENVELOPE_POINT_DIMENSION) {
			cerr << "Error: too many elements in first point of envelope: "
			     << aString << endl;
			exit(1);
		}
		testNumbers[getDimension()-1] = extractNumber(aString, index);
		skipSpace(aString, index, "\n \t,");
	}
	if (getDimension() <= 1) {
		cerr << "Error: not enough data for first envelope point: "
			  << aString << endl;
		exit(1);
	}


	// process the rest of the envelope points
   m_points.resize(getDimension());
	for (i=0; i<getDimension(); i++) {
		m_points[i].reserve(pointCount);
		m_points[i].resize(0);
		m_points[i].push_back(testNumbers[i]);
	}
	m_pointInterp.reserve(pointCount);
	m_pointInterp.resize(0);
	m_pointParameter.reserve(pointCount);
	m_pointParameter.resize(0);

	m_pointInterp[0] = '\0';
	m_pointParameter[0] = -9999;

	// no interpolation data allowed in envelope

	skipSpace(aString, index);
	if (aString[index] != ')') {
		cerr << "Error: unexpected continuation of data point at character "
			  << index + 1 << " in envelope: " << aString << endl;
		exit(1);
	}
	index++;

	// fill in the reset of the envelope point data
	for (i=1; i<pointCount; i++) {
		skipSpace(aString, index);
		if (aString[index] == '|') {
			if (stickIndex != -1) {
				cerr << "Error: only one stick point allowd in envelope: "
				     << aString << endl;
				exit(1);
			} else {
				stickIndex = i;
			}
			index++;
			skipSpace(aString, index);
		}
		if (aString[index] != '(') {
			cerr << "Error: at character " << index + 1 << " in envelope: "
			     << aString << endl;
			exit(1);
		}
		index++;

		m_points[0][i] = extractNumber(aString, index);
		m_points[0][i] /= getLength();
		if (m_points[0][i] < m_points[0][i-1]) {
			cerr << "Error: time values in envelope must always increase." << endl;
			cerr << "Check the time values: " << m_points[0][i-1] * getLength()
			      << " and " << m_points[0][i-1] * getLength()
			      << " around character " << index + 1 << " in envelope: "
			      << aString << endl;
			exit(1);
		}

		m_pointInterp[i] = '\0';
		m_pointParameter[i] = -9999;

		// no interpolation data allowed in envelope

		// get envelope point data
		j = 1;
		skipSpace(aString, index);
		if (aString[index] != ')') {
			for (j=1; j<getDimension(); j++) {
				m_points[j][i] = extractNumber(aString, index);
				skipSpace(aString, index);

				// no interpolation data allowed
			}
		}

		// sticky data is not allowed

		if (j < getDimension()) {
			cerr << "Error: not enough point data at character "
			     << index + 1 << " in envelope: " << aString << endl;
			exit(1);
		}

		skipSpace(aString, index);
		if (aString[index] != ')') {
			cerr << "Error: unexpected continuation of data point at character "
			     << index + 1 << " in envelope: " << aString << endl;
			exit(1);
		}
		index++;
		skipSpace(aString, index);

	} // end of for i loop

	if (aString[index] != ']') {
		cerr << "Error at end of envelope at character " << index + 1
			  << " in: " << aString << endl;
		exit(1);
	}

}



//////////////////////////////
//
// processMMAenv -- reads a Mathematica envelope into internal data format
//

void EnvelopeString::processMMAenv(const char* aString) {
	int length = strlen(aString);
	int i, j, index;
	stickIndex = -1;

	defaultInterpolation = 'L';
	defaultParameter = -9999;
	absoluteType = 't';

	// count the number of points in the envelope by counting the
	// number of open curly parenthesis and close curly parenthesis:
	int pointCount;
	int openP, closeP;
	openP = closeP = 0;
	for (i=0; i < length; i++) {
		if (aString[i] == '{') {
			openP++;
		} else if (aString[i] == '}') {
			closeP++;
		}
	}
	if (openP != closeP) {
		cerr << "Error: parenthesis do not match: " << aString << endl;
		exit(1);
	}
	pointCount = openP-1;  // extra outer parentheses
	if (pointCount < 1) {
		cerr << "Error: no points in envelope: " << aString << endl;
		exit(1);
	}

	// find the first coordinate of the last point in the envelope
	// to use in normalizing the time element of the envelope:
	index = length-1;
	while (aString[index] != '{' && index >= 0) {
		index--;
	}
	setLength(extractNumber(aString, index));

	// no interpolation indicators allowed.
	// no interpolation parameters allowed.

	index = 0;
	skipSpace(aString, index);

	// must be at start of envelope
	if (aString[index] != '{') {
		cerr << "Error at start of envelope: " << aString << endl;
		exit(1);
	}
	index++;
	skipSpace(aString, index);
	if (aString[index] != '{') {
		cerr << "Error at beginning of fist data point in envelope: "
			  << aString << endl;
		exit(1);
	}
	index++;


	// determine the dimension of an envelope point by checking
	// the number of elements in the first envelope point:
	dimension = 1;
	double testNumbers[MAX_ENVELOPE_POINT_DIMENSION];
	testNumbers[getDimension()-1] = extractNumber(aString, index);
	if (testNumbers[getDimension()-1] != 0.0) {
		cerr << "Error: first element in an envelope must be zero: "
			  << testNumbers[getDimension()-1] << " in " << aString << endl;
		exit(1);
	}
	skipSpace(aString, index);
	while (aString[index] != '}' && aString[index] != '\0') {
		dimension++;
		if (getDimension() >= MAX_ENVELOPE_POINT_DIMENSION) {
			cerr << "Error: too many elements in first point of envelope: "
			     << aString << endl;
			exit(1);
		}
		testNumbers[getDimension()-1] = extractNumber(aString, index);
		skipSpace(aString, index, "\n \t,");
	}


	if (getDimension() <= 1) {
		cerr << "Error: not enough data for first envelope point: "
			  << aString << endl;
		exit(1);
	}


	// process the rest of the envelope points
	m_points.resize(getDimension());
	for (i=0; i<getDimension(); i++) {
		m_points[i].reserve(pointCount);
		m_points[i].resize(0);
		m_points[i].push_back(testNumbers[i]);
	}
	m_pointInterp.reserve(pointCount);
	m_pointInterp.resize(0);
	m_pointParameter.reserve(pointCount);
	m_pointParameter.resize(0);

	m_pointInterp[0] = '\0';
	m_pointParameter[0] = -9999;

	// no interpolation data allowed in envelope

	skipSpace(aString, index);
	if (aString[index] != '}') {
		cerr << "Error: unexpected continuation of data point at character "
			  << index + 1 << " in envelope: " << aString << endl;
		exit(1);
	}
	index++;


	// fill in the reset of the envelope point data
	for (i=1; i<pointCount; i++) {
		skipSpace(aString, index);
		if (aString[index] == '|') {
			if (stickIndex != -1) {
				cerr << "Error: only one stick point allowed in envelope: "
			        << aString << endl;
				exit(1);
			} else {
				stickIndex = i;
			}
			index++;
			skipSpace(aString, index);
		}
		if (aString[index] != '{') {
			cerr << "Error: at character " << index + 1 << " in envelope: "
			     << aString << endl;
			exit(1);
		}
		index++;

		m_points[0][i] = extractNumber(aString, index);
		m_points[0][i] /= getLength();
		if (m_points[0][i] < m_points[0][i-1]) {
			cerr << "Error: time values in envelope must always increase." << endl;
			cerr << "Check the time values: " << m_points[0][i-1] * getLength()
			      << " and " << m_points[0][i-1] * getLength()
			      << " around character " << index + 1 << " in envelope: "
			      << aString << endl;
			exit(1);
		}

		m_pointInterp[i] = '\0';
		m_pointParameter[i] = -9999;

		// no interpolation data allowed in envelope

		// get envelope point data
		j = 1;
		skipSpace(aString, index);
		if (aString[index] != '}') {
			for (j=1; j<getDimension(); j++) {
				m_points[j][i] = extractNumber(aString, index);
				skipSpace(aString, index);
				// no interpolation data allowed
			}
		}

		// sticky data is not allowed

		if (j < getDimension()) {
			cerr << "Error: not enough point data at character "
			     << index + 1 << " in envelope: " << aString << endl;
			exit(1);
		}

		skipSpace(aString, index);
		if (aString[index] != '}') {
			cerr << "Error: unexpected continuation of data point at character "
			     << index + 1 << " in envelope: " << aString << endl;
			exit(1);
		}
		index++;
		skipSpace(aString, index);

	} // end of for i loop

	if (aString[index] != '}') {
		cerr << "Error at end of envelope at character " << index + 1
			  << " in: " << aString << endl;
		exit(1);
	}

}

//////////////////////////////
//
// processPLAINenv -- reads a PLAIN envelope into the internal data format
//	default value: dimension = -1
//

void EnvelopeString::processPLAINenv(const char* aString, int aDimension) {
	int length = strlen(aString);
	int i, j, index;
	stickIndex = -1;

	defaultInterpolation = 'L';
	defaultParameter = -9999;
	absoluteType = 't';

	vector<double> tempData;

	index = 0;
	skipSpace(aString, index);

	if (std::isalpha(aString[index])) {
		cerr << "Error at beginning of string: " << aString << endl;
		exit(1);
	}

	i = 0;
	while (aString[index] != '\0' && index < length) {
		tempData.push_back(extractNumber(aString, index));
		skipSpace(aString, index);
	}

	if (aDimension == -1) {
		dimension = 2;
	} else {
		dimension = aDimension;
	}
	if (getDimension() < 2) {
		cerr << "Error: not enough dimension to envelope points: "
			  << aString << endl;
		exit(1);
	}

	if (tempData.size() % getDimension() != 0) {
		cerr << "Error: PLAIN envelope must have enough points for dimension: "
			  << aString << endl;
		exit(1);
	}
	int pointCount = (int)tempData.size() / getDimension();
	setLength(tempData.at((int)tempData.size()-getDimension()));

	m_points.resize(getDimension());
	for (i=0; i<getDimension(); i++) {
		m_points[i].reserve(pointCount);
		m_points[i].resize(0);
	}
	m_pointInterp.reserve(pointCount);
	m_pointInterp.resize(0);
	m_pointParameter.reserve(pointCount);
	m_pointParameter.resize(0);

	for (i=0; i<pointCount; i++) {
		for (j=0; j<getDimension(); j++) {
			m_points[j][i] = tempData.at(i * getDimension() + j);
		}
		m_points[0][i] /= getLength();
		m_pointInterp[i] = '\0';
		m_pointParameter[i] = -9999;
	}

}



//////////////////////////////
//
// processSIGenv
//

void EnvelopeString::processSIGenv(const char* aString) {
	int length = strlen(aString);
	stickIndex = -1;                // global class value
	int tempStickIndex = -1;
	int i, j, k, index;


	// count the number of points in the envelope by counting
	// the number of semi-colons (';'), taking into account the sustain
	// point:
	int pointCount = 1;
	int sCount = 0;
	for (i=0; i<length; i++) {
		if (aString[i] == ';') pointCount++;
		if (std::tolower(aString[i]) == 's') {
			pointCount--;
			sCount++;
			if (sCount > 1) {
				cerr << "Error: too many letter \"s\"es found in envleope: "
			        << aString << endl;
				exit(1);
			}
		}
	}
	if (pointCount <= 1) {
		cerr << "Error: not enough points in envelope: " << aString << endl;
		exit(1);
	}


	// find first coordinate of the last point in envelope to use
	// in normalizing the time element of the envelope:
	index = length-1;
	while (aString[index] != ';' && index >= 0) {
		index--;
	}
	setLength(extractNumber(aString, index));


	// check for a default interpolation type
	defaultInterpolation = 'L';
	defaultParameter = -9999;
	index = 0;
	skipSpace(aString, index);
	if (std::isalpha(aString[index])) {
		validateInterpolation(aString[index]);
		defaultInterpolation = std::toupper(aString[index]);
		index++;
		skipSpace(aString, index);
		if (std::isdigit(aString[index])) {
			defaultParameter = extractNumber(aString, index);
			skipSpace(aString, index);
		}
	}

	// must be at start of envelope
	if (aString[index] != '(') {
		cerr << "Error at start of envelope: " << aString << endl;
		exit(1);
	}


	// determine the dimension of an envelope point by checking
	// the number of elements in the first envelope point:
	dimension = 1;
	double testNumbers[MAX_ENVELOPE_POINT_DIMENSION];
	testNumbers[getDimension()-1] = extractNumber(aString, index);
	if (testNumbers[getDimension()-1] != 0.0) {
		cerr << "Error: first element in an envelope must be zero: "
			  << testNumbers[getDimension()-1] << " in " << aString << endl;
		exit(1);
	}
	while (aString[index] != ';' && !std::isalpha(aString[index]) &&
			aString[index] != ')' && aString[index] != '\0' ) {
		dimension++;
		if (getDimension() >= MAX_ENVELOPE_POINT_DIMENSION) {
			cerr << "Error: too many elements in first point of envelope: "
			     << aString << endl;
			exit(1);
		}
		testNumbers[getDimension()-1] = extractNumber(aString, index);
		skipSpace(aString, index, "\n \t,");
	}
	if (getDimension() <= 1) {
		cerr << "Error: not enough data for first envelope point: "
			  << getDimension() << "  "
			  << aString << endl;
		exit(1);
	}


	// process the rest of the envleope points
	m_points.resize(getDimension());
	for (i=0; i<getDimension(); i++) {
		m_points[i].reserve(pointCount);
		m_points[i].resize(0);
		m_points[i].push_back(testNumbers[i]);
	}
	m_pointInterp.reserve(pointCount);
	m_pointInterp.resize(0);
	m_pointParameter.reserve(pointCount);
	m_pointParameter.resize(0);

	m_pointInterp[0] = '\0';
	m_pointParameter[0] = -9999;

	// check for any interpolation data in first point
	if (std::isalpha(aString[index])) {
		validateInterpolation(aString[index]);
		m_pointInterp[0] = aString[index];
		index++;
		// skip any whitespace and see if next char is ';' or ')'
		// otherwise a parameter for interpolation
		skipSpace(aString, index);
		if (aString[index] == ';' || aString[index] == ')') {
			m_pointParameter[0] = -9998;
		} else {
			m_pointParameter[0] = extractNumber(aString, index);
		}
	} else {
		m_pointInterp[0] = '\0';
	}


	// fill in the rest of the envelope point data
	for (i=1; i<pointCount; i++) {

		skipSpace(aString, index);
		if (!(aString[index] == ';' || aString[index] == ')')) {
			cerr << "Error: unexpected continuation of point data: "
			     << aString << endl;
			exit(1);
		}
		index++;

		skipSpace(aString, index);
		if (std::tolower(aString[index]) == 's') {
			if (tempStickIndex != -1) {
				cerr << "Error: can have only one stick point in envelope: "
			        << aString << endl;
				exit(1);
			} else {
				tempStickIndex = i;
			}
			index++;
			skipSpace(aString, index);
			if (!(aString[index] == ';' || aString[index] == ')')) {
				cerr << "Error: unexpected continuation of point data: "
			        << aString << endl;
				exit(1);
			}
			index++;
		}

		m_points[0][i] = extractNumber(aString, index);
		m_points[0][i] /= getLength();
		if (m_points[0][i] < m_points[0][i-1]) {
			cerr << "Error: time values in envelope must always increase." << endl;
			cerr << "Check the time values: " << m_points[0][i-1] << " and "
			     << m_points[0][i] << " in the envelope: " << aString << endl;
			exit(1);
		}

		m_pointInterp[i] = '\0';
		m_pointParameter[i] = 0.0;

		// check for any interpolation data.
		if (std::isalpha(aString[index])) {
			m_pointInterp[i] = aString[index];
			// skip any whitespace and see if next char is ';' or ')'
			// otherwise a parameter for interpolation
			while (index < length && std::isspace(aString[index])) {
				index++;
			}
			if (aString[index] == ';' || aString[index] == ')') {
				m_pointParameter[i] = 0.0;
			} else {
				m_pointParameter[i] = extractNumber(aString, index);
			}
		}

		j = 1;
		if (aString[index] != ';' && aString[index] != ')') {
			for (j=1; j<getDimension(); j++) {
				m_points[j][i] = extractNumber(aString, index);

				// check for any interpolation data.
				if (std::isalpha(aString[index])) {
					validateInterpolation(aString[index]);
					m_pointInterp[i] = aString[index];
					index++;
					// skip any whitespace and see if next char is ';' or ')'
					// otherwise a parameter for interpolation
					skipSpace(aString, index);
					if (aString[index] == ';' || aString[index] == ')') {
						m_pointParameter[i] = -9998;
					} else {
						m_pointParameter[i] = extractNumber(aString, index);
					}
				}
			}
		}

		// allow for "sticky data" which carries over if not specified:
		if (j < getDimension()) {
			for (k=j; k<getDimension(); k++) {
				m_points[k][i] = m_points[k][i-1];
			}
		}

	} // end of for i loop

	if (aString[index] != ')') {
		cerr << "Error at end of envelope: " << aString << endl;
	}
	index++;

	if (tempStickIndex >= 0) {
		setStickIndex(tempStickIndex-1);
	}

	if (aString[index] == '\0') {
		return;
	} else {
		skipSpace(aString, index);
	}

	if (aString[index] == '\0') {
		return;
	} else if (std::tolower(aString[index]) == 't') {
		absoluteType = 't';
	} else if (std::tolower(aString[index]) == 'm') {
		absoluteType = 'm';
	} else {
		cerr << "Error: unknown characters after end of envelope: "
			  << aString << endl;
		exit(1);
	}

}



//////////////////////////////
//
// EnvelopeString::adjustForStick --
//

void EnvelopeString::adjustForStick(void) {
	if (stickIndex <= 0) {
		return;
	}

	int i = 0;
	if (m_pointInterp[i] == m_pointInterp[(int)m_pointInterp.size()-1]) {
		i = (int)m_pointInterp.size()-1;
		while (i>0 && m_pointInterp[i] <= 0) {
			m_pointInterp[i+1] = m_pointInterp[i];
			m_pointParameter[i+1] = m_pointInterp[i];
			for (int j=0; j<getDimension(); j++) {
				m_points[j][i+1] = m_points[j][i];
			}
			i--;
		}
	}

	m_points[0][stickIndex] = fabs(m_points[0][stickIndex]);

}



//////////////////////////////
//
// EnvelopeString::skipSpace --
//	default value: spaceString = "\n \t,"
//

void EnvelopeString::skipSpace(const char* string, int& index,
		const char* spaceString) {
	int length = strlen(spaceString);
	int foundSpace;
	int i;

	while (1) {
		foundSpace = 0;
		for (i=0; i<length; i++) {
			if (string[index] == spaceString[i]) {
				foundSpace = 1;
				break;
			}
		}
		if (foundSpace) {
			index++;
		} else {
			break;
		}
	}
}


//////////////////////////////
//
// EnvelopeString::validateInterpolation --
//


int EnvelopeString::validateInterpolation(char anInterp) {
	switch (std::tolower(anInterp)) {
		case 'l':
		case 'g':
		case 'c':
			return 1;
		default:
			cerr << "Error: unknown interpolation: " << anInterp << endl;
			exit(1);
	}
		return 0;
}



