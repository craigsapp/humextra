//
// Copyright 2013 by Craig Stuart Sapp, All Rights Reserved.
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Fri Mar 29 15:14:19 PDT 2013
// Last Modified: Fri Mar 29 15:14:24 PDT 2013
// Filename:      ...sig/src/sigInfo/HumdrumFileSet.cpp
// Web Address:   http://sig.sapp.org/src/sigInfo/HumdrumFileSet.cpp
// Syntax:        C++
//
// Description:   Higher-level functions for processing Humdrum files.
//                Inherits HumdrumFileSetBasic and adds rhythmic and other
//                types of analyses to the HumdrumFileSet class.
//

#include "HumdrumFileSet.h"
#include "PerlRegularExpression.h"

#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;


//////////////////////////////
//
// HumdrumFileSet::HumdrumFileSet --
//

HumdrumFileSet::HumdrumFileSet(void) {
	data.setSize(10000);
	data.setGrowth(90000);
	data.setSize(0);
}



//////////////////////////////
//
// HumdrumFileSet::~HumdrumFileSet --
//

HumdrumFileSet::~HumdrumFileSet() {
	clear();
}



//////////////////////////////
//
// HumdrumFileSet::clear -- Remove all Humdrum file content from set.
//

void HumdrumFileSet::clear(void) {
	int i;
	for (i=0; i<data.getSize(); i++) {
		delete data[i];
		data[i] = NULL;
	}
	data.setSize(0);
}



//////////////////////////////
//
// HumdrumFileSet::getSize -- Return the number of Humdrum files in the
//     set.
//

int HumdrumFileSet::getSize(void) {
	return data.getSize();
}



//////////////////////////////
//
// HumdrumFileSet::operator[] -- Return a HumdrumFile.
//

HumdrumFile& HumdrumFileSet::operator[](int index) {
	return *(data[index]);
}



//////////////////////////////
//
// HumdrumFileSet::read -- Returns the total number of segments
//

int HumdrumFileSet::read(const string& filename) {
	return read(filename.data());
}

int HumdrumFileSet::read(const char* filename) {
	clear();
	return readAppend(filename);
}

int HumdrumFileSet::read(istream& inStream) {
	clear();
	return readAppend(inStream);
}

int HumdrumFileSet::read(Options& options) {
	clear();
	int i;
	int numinputs = options.getArgumentCount();
	HumdrumFileSet& infiles = *this;

	if (numinputs < 1) {
		infiles.read(cin);
	} else {
		for (i=0; i<numinputs; i++) {
			infiles.readAppend(options.getArg(i+1));
		}
	}

	return infiles.getCount();
}



//////////////////////////////
//
// HumdrumFileSet::readAppend -- Returns the total number of segments
//    Adds each new HumdrumFile segment to the end of the current data.
//

int HumdrumFileSet::readAppend(const string& filename) {
	return readAppend(filename.data());
}


int HumdrumFileSet::readAppend(const char* filename) {

#ifdef USING_URI
	stringstream instream;
	if (strstr(filename, "://") != NULL) {
		if (strncmp(filename, "http://", strlen("http://")) == 0) {
			readAppendFromHttpURI(instream, filename);
			return readAppend(instream);
		}
		if (strncmp(filename, "jrp://", strlen("jrp://")) == 0) {
			readAppendFromJrpURI(instream, filename);
			return readAppend(instream, filename);
		}
		if (strncmp(filename, "humdrum://", strlen("humdrum://")) == 0) {
			readAppendFromHumdrumURI(instream, filename);
			return readAppend(instream);
		}
		if (strncmp(filename, "hum://", strlen("hum://")) == 0) {
			readAppendFromHumdrumURI(instream, filename);
			return readAppend(instream);
		}
		if (strncmp(filename, "h://", strlen("h://")) == 0) {
			readAppendFromHumdrumURI(instream, filename);
			return readAppend(instream);
		}
	}
#endif

	ifstream infile;
	infile.open(filename, ios::in);

	if (!infile) {
		cerr << "Error: could not open file: " << filename << endl;
		exit(1);
	}

	if (infile.peek() == '%') {
		stringstream embeddedData;
		HumdrumFile::extractEmbeddedDataFromPdf(embeddedData, infile);
		return readAppend(embeddedData);
	}

	return readAppend(infile, filename);
}


// !!!!SEGMENT: filename

int HumdrumFileSet::readAppend(istream& inStream, const char* filename) {
	PerlRegularExpression pre;
	int contentQ   = 0;
	int exclusiveQ = 0;
	char line[1123123] = {0};
	stringstream* inbuffer;
	char tfilename[123123] = {0};
	if (strrchr(filename, '/') != NULL) {
		filename = strrchr(filename, '/') + 1;
	}
	strcpy(tfilename, filename);
	inbuffer = new stringstream;
	while (!inStream.eof()) {
		line[0] = '\0';
		inStream.getline(line, 321321);
		if (inStream.eof() && (strlen(line) == 0)) {
			break;
		}
		if (strncmp(line, "**", 2) == 0) {
			exclusiveQ++;
		}
		if (pre.search(line, "^!!!!SEGMENT:\\s*(.*)\\s*$", "")) {
			if (contentQ != 0) {
				if (strlen(tfilename) == 0) {
					strcpy(tfilename, filename);
				}
				appendHumdrumFileContent(tfilename, *inbuffer);
				delete inbuffer;
				inbuffer = new stringstream;
				strcpy(tfilename, "");
			}
			strcpy(tfilename, pre.getSubmatch(1));
			contentQ = 0;
			exclusiveQ = 0;
		} else if (exclusiveQ == 2) {
			// cannot have more than one exclusive interpretation in a
			// HumdrumFile data structure, so split the data at this
			// point into a new HumdrumFile segement, assigning an
			// empty filename.
			appendHumdrumFileContent("", *inbuffer);
			delete inbuffer;
			inbuffer = new stringstream;
			exclusiveQ = 1;
			contentQ = 1;
			*inbuffer << line << "\n";
		} else {
			contentQ = 1;
			*inbuffer << line << "\n";
		}
	}

	if (contentQ) {
		// store last segment
		appendHumdrumFileContent(filename, *inbuffer);
	}

	delete inbuffer;
	return getSize();
}


void HumdrumFileSet::appendHumdrumFileContent(const char* filename,
		stringstream& inbuffer) {
	HumdrumFile* newfile;
	newfile = new HumdrumFile;
	HumdrumFile& infile = *newfile;
	infile.setFilename(filename);
	infile.read(inbuffer);
	data.append(newfile);
}


#ifdef USING_URI

//////////////////////////////
//
// readFromHumdrumURI -- Read a Humdrum file from an humdrum:// web address
//
// Example:
// maps: humdrum://osu/classical/haydn/london/sym099a.krn
// into:
// http://kern.ccarh.org/cgi-bin/ksdata?file=sym099a.krn&l=/osu/classical/haydn/london&format=kern
//

void HumdrumFileSet::readAppendFromHumdrumURI(stringstream& inputstream,
		const char* humdrumaddress) {
	const char* ptr = humdrumaddress;
	// skip over the staring portion of the address:
	if (strncmp(ptr, "humdrum://", strlen("humdrum://")) == 0) {
		ptr += strlen("humdrum://");
	} else if (strncmp(ptr, "hum://", strlen("hum://")) == 0) {
		ptr += strlen("hum://");
	} else if (strncmp(ptr, "h://", strlen("h://")) == 0) {
		ptr += strlen("h://");
	}
	Array<char> location;
	location.setSize(strlen(ptr)+1);
	location.allowGrowth(0);
	strcpy(location.getBase(), ptr);

	Array<char> filename;
	filename.setSize(1);
	filename[0] = '\0';
	filename.setSize(0);

	strcpy(location.getBase(), ptr);

	char* pot;
	if ((pot = strrchr(location.getBase(), '/')) != NULL) {
		*pot = '\0';
		pot++;
		filename.setSize(strlen(pot)+1);
		strcpy(filename.getBase(), pot);
		filename.allowGrowth(0);
	} else {
		filename = location;
		location.setSize(2);
		strcpy(location.getBase(), "/");
	}

	stringstream httprequest;
	httprequest << "http://" << "kern.ccarh.org";
	httprequest << "/cgi-bin/ksdata?";
	if (strstr(filename.getBase(), ".krn") != NULL) {
		httprequest << "l=" << location.getBase();
		httprequest << "&file=" << filename.getBase();
		httprequest << "&format=kern";
	} else {
		httprequest << "l=" << ptr;
	}
	httprequest << ends;

	readAppendFromHttpURI(inputstream, httprequest.str().c_str());
}



//////////////////////////////
//
// readAppendFromJrpURI -- Read a Humdrum file from a jrp:// web-style address
//
// Example:
// maps:
//    jrp://Jos2721-La_Bernardina
// into:
//    http://jrp.ccarh.org/cgi-bin/jrp?a=humdrum&f=Jos2721-La_Bernardina
//

void HumdrumFileSet::readAppendFromJrpURI(stringstream& inputstream,
		const char* jrpaddress) {
	const char* ptr = jrpaddress;
	// skip over the staring portion of the address:
	if (strncmp(ptr, "jrp://", strlen("jrp://")) == 0) {
		ptr += strlen("jrp://");
	} else if (strncmp(ptr, "jrp:", strlen("jrp:")) == 0) {
		ptr += strlen("jrp:");
	}

	stringstream httprequest;
	httprequest << "http://" << "jrp.ccarh.org";
	httprequest << "/cgi-bin/jrp?a=humdrum&f=";
	httprequest << ptr;
	httprequest << ends;

	const char* filename = jrpaddress;
	readAppendFromHttpURI(inputstream, httprequest.str().c_str(), filename);
}



//////////////////////////////
//
// readAppendFromHttpURI -- Read a Humdrum file from an http:// web address
//    Default value: filename = NULL.
//

void HumdrumFileSet::readAppendFromHttpURI(stringstream& inputstream,
		const char* webaddress, const char* filename) {
	Array<char> hostname;

	Array<char> location;
	location.setSize(0);

	const char* ptr = webaddress;

	if (strncmp(webaddress, "http://", strlen("http://")) == 0) {
		// remove the "http://" portion of the webaddress
		ptr += strlen("http://");
	}

	hostname.setSize(strlen(ptr)+1);
	hostname.setGrowth(0);
	strcpy(hostname.getBase(), ptr);
	char* pot;
	if ((pot = strchr(hostname.getBase(), '/')) != NULL) {
		*pot = '\0';
		hostname.setSize(strlen(hostname.getBase())+1);
	}

	const char* ptr3;
	if ((ptr3 = strchr(ptr, '/')) != NULL) {
		location.setSize(strlen(ptr3)+1);
		strcpy(location.getBase(), ptr3);
		location.allowGrowth(0);
	}
	if (location.getSize() == 0) {
		location.setSize(2);
		location.allowGrowth(0);
		strcpy(location.getBase(), "/");
	}

	const char* ptr2 = NULL;
	// const char* newfilename = filename;
	if ((filename != NULL) && ((ptr2 = strrchr(filename, '/')) != NULL)) {
		// newfilename = ptr2+1;
	}

	char newline[3] = {0x0d, 0x0a, 0};

	stringstream request;
	request << "GET "   << location.getBase() << " HTTP/1.1" << newline;
	request << "Host: " << hostname.getBase() << newline;
	request << "User-Agent: HumdrumFile Downloader 1.0 ("
			  << __DATE__ << ")" << newline;
	request << "Connection: close" << newline;  // this line is necessary
	request << newline;
	request << ends;

	// cout << "HOSTNAME: " << hostname.getBase() << endl;
	// cout << "LOCATION: " << location.getBase() << endl;
	// cout << request.str().c_str() << endl;
	// cout << "-------------------------------------------------" << endl;

	int socket_id = open_network_socket(hostname.getBase(), 80);
	if (::write(socket_id, request.str().c_str(), strlen(request.str().c_str())) == -1) {
		exit(-1);
	}
	#define URI_BUFFER_SIZE (10000)
	char buffer[URI_BUFFER_SIZE];
	int message_len;
	stringstream header;
	int foundcontent = 0;
	int i;
	int newlinecounter = 0;

	// read the response header:
	while ((message_len = ::read(socket_id, buffer, 1)) != 0) {
		header << buffer[0];
		if ((buffer[0] == 0x0a) || (buffer[0] == 0x0d)) {
					newlinecounter++;
		} else {
					newlinecounter = 0;
		}
		if (newlinecounter == 4) {
			foundcontent = 1;
			break;
		}
	}
	if (foundcontent == 0) {
		cerr << "Funny error trying to read server response" << endl;
		exit(1);
	}

	// now read the size of the rest of the data which is expected
	int datalength = -1;

	// also, check for chunked transfer encoding:

	int chunked = 0;

	header << ends;
	// cout << header.str().c_str() << endl;
	// cout << "-------------------------------------------------" << endl;
	while (header.getline(buffer, URI_BUFFER_SIZE)) {
		int len = strlen(buffer);
		for (i=0; i<len; i++) {
			buffer[i] = std::tolower(buffer[i]);
		}
		if (strstr(buffer, "content-length") != NULL) {
			for (i=14; i<len; i++) {
				if (std::isdigit(buffer[i])) {
					sscanf(&buffer[i], "%d", &datalength);
					if (datalength == 0) {
						cerr << "Error: no data found for URI, probably invalid\n";
						exit(1);
					}
					break;
				}
			}
		} else if ((strstr(buffer, "transfer-encoding") != NULL) &&
			(strstr(buffer, "chunked") != NULL)) {
			chunked = 1;
		}
		// if (datalength >= 0) {
		//    break;
		// }
	}

	// once the length of the remaining data is known (or not), read it:
	if (datalength > 0) {
		getFixedDataSize(socket_id, datalength, inputstream, buffer,
				URI_BUFFER_SIZE);

	} else if (chunked) {
		int chunksize;
		int totalsize = 0;
		do {
			chunksize = getChunk(socket_id, inputstream, buffer, URI_BUFFER_SIZE);
			totalsize += chunksize;
		} while (chunksize > 0);
		if (totalsize == 0) {
			cerr << "Error: no data found for URI (probably invalid)\n";
			exit(1);
		}
	} else {
		// if the size of the rest of the data cannot be found in the
		// header, then just keep reading until done (but this will
		// probably cause a 5 second delay at the last read).
		while ((message_len = ::read(socket_id, buffer, URI_BUFFER_SIZE)) != 0) {
			if (foundcontent) {
				inputstream.write(buffer, message_len);
			} else {
				for (i=0; i<message_len; i++) {
					if (foundcontent) {
						inputstream << buffer[i];
					} else {
						header << buffer[i];
						if ((buffer[i] == 0x0a) || (buffer[i] == 0x0d)) {
							newlinecounter++;
						} else {
							newlinecounter = 0;
						}
						if (newlinecounter == 4) {
							foundcontent = 1;
							continue;
						}
					}
				}
			}
		}
	}

	close(socket_id);

	// cout << "CONTENT:=======================================" << endl;
	// cout << inputstream.str().c_str();
	// cout << "===============================================" << endl;
	// HumdrumFileSet::read(inputstream);
}



//////////////////////////////
//
//  HumdrumFileSet::getChunk --
//
// http://en.wikipedia.org/wiki/Chunked_transfer_encoding
// http://tools.ietf.org/html/rfc2616
//
// Chunk Format
//
// If a Transfer-Encoding header with a value of chunked is specified in
// an HTTP message (either a request sent by a client or the response from
// the server), the body of the message is made of an unspecified number
// of chunks ending with a last, zero-sized, chunk.
//
// Each non-empty chunk starts with the number of octets of the data it
// embeds (size written in hexadecimal) followed by a CRLF (carriage
// return and linefeed), and the data itself. The chunk is then closed
// with a CRLF. In some implementations, white space chars (0x20) are
// padded between chunk-size and the CRLF.
//
// The last chunk is a single line, simply made of the chunk-size (0),
// some optional padding white spaces and the terminating CRLF. It is not
// followed by any data, but optional trailers can be sent using the same
// syntax as the message headers.
//
// The message is finally closed by a last CRLF combination.

int HumdrumFileSet::getChunk(int socket_id, stringstream& inputstream,
		char* buffer, int bufsize) {
	int chunksize = 0;
	int message_len;
	char digit[2] = {0};
	int founddigit = 0;

	// first read the chunk size:
	while ((message_len = ::read(socket_id, buffer, 1)) != 0) {
		if (isxdigit(buffer[0])) {
			digit[0] = buffer[0];
			chunksize = (chunksize << 4) | strtol(digit, NULL, 16);
			founddigit = 1;
		} else if (founddigit) {
			break;
		} // else skipping whitespace before chunksize
	}
	if ((chunksize <= 0) || (message_len == 0)) {
		// next chunk is zero, so no more primary data0:w
		return 0;
	}

	// read the 0x0d and 0x0a characters which are expected (required)
	// after the size of chunk size:
	if (buffer[0] != 0x0d) {
		cerr << "Strange error occurred right after reading a chunk size" << endl;
		exit(1);
	}

	// now expect 0x0a:
	message_len = ::read(socket_id, buffer, 1);
	if ((message_len == 0) || (buffer[0] != 0x0a)) {
		cerr << "Strange error after reading newline at end of chunk size"<< endl;
		exit(1);
	}

	return getFixedDataSize(socket_id, chunksize, inputstream, buffer, bufsize);
}



//////////////////////////////
//
// getFixedDataSize -- read a know amount of data from a socket.
//

int HumdrumFileSet::getFixedDataSize(int socket_id, int datalength,
		stringstream& inputstream, char* buffer, int bufsize) {
	int readcount = 0;
	int readsize;
	int message_len;

	while (readcount < datalength) {
		readsize = bufsize;
		if (readcount + readsize > datalength) {
			readsize = datalength - readcount;
		}
		message_len = ::read(socket_id, buffer, readsize);
		if (message_len == 0) {
			// shouldn't happen, but who knows...
			break;
		}
		inputstream.write(buffer, message_len);
		readcount += message_len;
	}

	return readcount;
}



//////////////////////////////
//
// HumdrumFileSet::prepare_address -- Store a computer name, such as
//    www.google.com into a sockaddr_in structure for later use in
//    open_network_socket.
//

void HumdrumFileSet::prepare_address(struct sockaddr_in *address,
		const char *hostname, unsigned short int port) {

	memset(address, 0, sizeof(struct sockaddr_in));
	struct hostent *host_entry;
	host_entry = gethostbyname(hostname);

	if (host_entry == NULL) {
		cerr << "Could not find address for" << hostname << endl;
		exit(1);
	}

	// copy the address to the sockaddr_in struct.
	memcpy(&address->sin_addr.s_addr, host_entry->h_addr_list[0],
			host_entry->h_length);

	// set the family type (PF_INET)
	address->sin_family = host_entry->h_addrtype;
	address->sin_port = htons(port);
}



//////////////////////////////
//
// open_network_socket -- Open a connection to a computer on the internet.
//    Intended for downloading a Humdrum file from a website.
//

int HumdrumFileSet::open_network_socket(const char *hostname,
		unsigned short int port) {
	int inet_socket;                 // socket descriptor
	struct sockaddr_in servaddr;     // IP/port of the remote host

	prepare_address(&servaddr, hostname, port);

	// socket(domain, type, protocol)
	//    domain   = PF_INET(internet/IPv4 domain)
	//    type     = SOCK_STREAM(tcp) *
	//    protocol = 0 (only one SOCK_STREAM type in the PF_INET domain
	inet_socket = socket(PF_INET, SOCK_STREAM, 0);

	if (inet_socket < 0) {
		// socket returns -1 on error
		cerr << "Error opening socket to computer " << hostname << endl;
		exit(1);
	}

	// connect(sockfd, serv_addr, addrlen)
	if (connect(inet_socket, (struct sockaddr *)&servaddr,
			sizeof(struct sockaddr_in)) < 0) {
		// connect returns -1 on error
		cerr << "Error opening connection to coputer: " << hostname << endl;
		exit(1);
	}

	return inet_socket;
}

#endif



