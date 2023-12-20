//
// Programmer:    Craig Stuart Sapp <craig@ccrma.stanford.edu>
// Creation Date: Tue Dec 11 16:09:32 PST 2012
// Last Modified: Tue Dec 11 16:09:38 PST 2012
// Last Modified: Fri Mar 11 21:26:18 PST 2016 Changed to STL
// Filename:      ...sig/src/sigInfo/HumdrumStream.cpp
// Web Address:   http://sig.sapp.org/src/sigInfo/HumdrumStream.cpp
// Syntax:        C++
//
// Description:   Higher-level functions for processing Humdrum files.
//                Inherits HumdrumStreamBasic and adds rhythmic and other
//                types of analyses to the HumdrumStream class.
//

#include "HumdrumStream.h"
#include "PerlRegularExpression.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;

#ifdef USING_URI
	#include <sys/types.h>   /* socket, connect */
	#include <sys/socket.h>  /* socket, connect */
	#include <netinet/in.h>  /* htons           */
	#include <netdb.h>       /* gethostbyname   */
	#include <unistd.h>      /* read, write     */
	#include <string.h>      /* memcpy          */
#endif



//////////////////////////////
//
// HumdrumStream::HumdrumStream --
//

HumdrumStream::HumdrumStream(void) {
	curfile = -1;
}

HumdrumStream::HumdrumStream(char** list) {
	curfile = -1;
	setFileList(list);
}

HumdrumStream::HumdrumStream(const vector<string>& list) {
	curfile = -1;
	setFileList(list);
}

HumdrumStream::HumdrumStream(Options& options) {
	curfile = -1;
	vector<string> list;
	options.getArgList(list);
	setFileList(list);
}



//////////////////////////////
//
// HumdrumStream::clear -- reset the contents of the class.
//

void HumdrumStream::clear(void) {
	curfile = 0;
	filelist.resize(0);
	universals.resize(0);
	newfilebuffer.resize(0);
}



//////////////////////////////
//
// HumdrumStream::setFileList --
//

int HumdrumStream::setFileList(char** list) {
	filelist.reserve(1000);
	filelist.resize(0);
	int i = 0;
	while (list[i] != NULL) {
		filelist.push_back(list[i]);
		i++;
	}
	return i;
}


int HumdrumStream::setFileList(const vector<string>& list) {
	filelist = list;
	return (int)list.size();
}



//////////////////////////////
//
// HumdrumStream::read -- alias for getFile.
//

int HumdrumStream::read(HumdrumFile& infile) {
	return getFile(infile);
}



//////////////////////////////
//
// HumdrumStream::eof -- returns true if there is no more segements
//     to read from the input source(s).
//

int HumdrumStream::eof(void) {
	istream* newinput = NULL;

	// Read HumdrumFile contents from:
	// (1) Current ifstream if open
	// (2) Next filename if ifstream is done
	// (3) cin if no ifstream open and no filenames

	// (1) Is an ifstream open?, then yes, there is more data to read.
	if (instream.is_open() && !instream.eof()) {
		return 0;
	}

	// (1b) Is the URL data buffer open?
	else if (urlbuffer.str() != "") {
		return 0;
	}

	// (2) If ifstream is closed but there is a file to be processed,
	// load it into the ifstream and start processing it immediately.
	else if ((filelist.size() > 0) && (curfile < (int)filelist.size()-1)) {
		return 0;
	} else {
		// no input fstream open and no list of files to process, so
		// start (or continue) reading from standard input.
		if (curfile < 0) {
			// but only read from cin if no files have previously been read
			newinput = &cin;
		}
		if ((newinput != NULL) && newinput->eof()) {
			return 1;
		}
	}

	return 1;
}



//////////////////////////////
//
// HumdrumStream::getFile -- fills a HumdrumFile class with content
//    from the input stream or next input file in the list.  Returns
//    true if content was extracted, fails if there is no more HumdrumFiles
//    in the input stream.
//

int HumdrumStream::getFile(HumdrumFile& infile) {
	infile.clear();
	istream* newinput;

restarting:

	newinput = NULL;

	if (urlbuffer.eof()) {
		// If the URL buffer is at its end, clear the buffer.
		urlbuffer.str("");
	}

	// Read HumdrumFile contents from:
	// (1) Current ifstream if open
	// (2) Next filename if ifstream is done
	// (3) cin if no ifstream open and no filenames

	// (1) Is an ifstream open?
	if (instream.is_open() && !instream.eof()) {
		newinput = &instream;
	}

	// (1b) Is the URL data buffer open?
	else if (urlbuffer.str() != "") {
		urlbuffer.clear();
		newinput = &urlbuffer;
	}

	// (2) If ifstream is closed but there is a file to be processed,
	// load it into the ifstream and start processing it immediately.
	else if (((int)filelist.size() > 0) && (curfile < (int)filelist.size()-1)) {
		curfile++;
		if (instream.is_open()) {
			instream.close();
		}
		if (strstr(filelist[curfile].c_str(), "://") != NULL) {
			// The next file to read is a URL/URI, so buffer the
			// data from the internet and start reading that instead
			// of reading from a file on the hard disk.
			fillUrlBuffer(urlbuffer, filelist[curfile].c_str());
			infile.setFilename(filelist[curfile].c_str());
			goto restarting;
		}
		instream.open(filelist[curfile].c_str());
		infile.setFilename(filelist[curfile].c_str());
		if (!instream.is_open()) {
			// file does not exist or cannot be opened close
			// the file and try luck with next file in the list
			// (perhaps given an error or warning?).
			infile.setFilename("");
			instream.close();
			goto restarting;
		}
		newinput = &instream;
	} else {
		// no input fstream open and no list of files to process, so
		// start (or continue) reading from standard input.
		if (curfile < 0) {
			// but only read from cin if no files have previously been read
			newinput = &cin;
		}
	}

	// At this point the newinput istream is set to read from the given
	// file or from standard input, so start reading Humdrum content.
	// If there is "newfilebuffer" content, then set the filename of the
	// HumdrumFile to that value.

	if (newfilebuffer.size() > 0) {
		// store the filename for the current HumdrumFile being read:
		PerlRegularExpression pre;
		if (pre.search(newfilebuffer,
				"^!!!!SEGMENT\\s*([+-]?\\d+)?\\s*:\\s*(.*)\\s*$", "i")) {
			if ((int)strlen(pre.getSubmatch(1)) > 0) {
				infile.setSegmentLevel(atoi(pre.getSubmatch()));
			} else {
				infile.setSegmentLevel(0);
			}
			infile.setFilename(pre.getSubmatch(2));
		} else if ((curfile >=0) && (curfile < (int)filelist.size())
				&& (filelist.size() > 0)) {
			infile.setFilename(filelist[curfile].c_str());
		} else {
			// reading from standard input, but no name.
		}
	}

	if (newinput == NULL) {
		// something strange happened, or no more files to read.
		return 0;
	}

	stringstream buffer;
	int foundUniversalQ = 0;

	// Start reading the input stream.  If !!!!SEGMENT: universal comment
	// is found, then store that line in newfilebuffer and return the
	// newly read HumdrumFile.  If other universal comments are found, then
	// overwrite the old universal comments here.

	int addedFilename = 0;
	//int searchName = 0;
	int dataFoundQ = 0;
	int starstarFoundQ = 0;
	int starminusFoundQ = 0;
	if (newfilebuffer.size() < 4) {
		//searchName = 1;
	}
	char templine[123123] = {0};

	if (newinput->eof()) {
		if (curfile < (int)filelist.size()-1) {
			curfile++;
			goto restarting;
		}
		// input stream is close and there is no more files to process.
		return 0;
	}

	istream& input = *newinput;

	// if the previous line from the last read starts with "**"
	// then treat it as part of the current file.
	if ((newfilebuffer.size() > 1) &&
		 (strncmp(newfilebuffer.c_str(), "**", 2)) == 0) {
		buffer << newfilebuffer << "\n";
		newfilebuffer = "";
		starstarFoundQ = 1;
	}

	while (!input.eof()) {
		input.getline(templine, 123123, '\n');
		if ((!dataFoundQ) &&
				(strncmp(templine, "!!!!SEGMENT", strlen("!!!!SEGMENT")) == 0)) {
			string tempstring;
			tempstring = templine;
			PerlRegularExpression pre;
			if (pre.search(tempstring,
					"^!!!!SEGMENT\\s*([+-]?\\d+)?\\s*:\\s*(.*)\\s*$", "i")) {
				if (strlen(pre.getSubmatch(1)) > 0) {
					infile.setSegmentLevel(atoi(pre.getSubmatch()));
				} else {
					infile.setSegmentLevel(0);
				}
				infile.setFilename(pre.getSubmatch(2));
			}
			continue;
		}

		if (strncmp(templine, "**", 2) == 0) {
			if (starstarFoundQ == 1) {
				newfilebuffer = templine;
				// already found a **, so this one is defined as a file
				// segment.  Exit from the loop and process the previous
				// content, waiting until the next read for to start with
				// this line.
				break;
			}
			starstarFoundQ = 1;
		}

		if (input.eof() && (strcmp(templine, "") == 0)) {
			// No more data coming from current stream, so this is
			// the end of the HumdrumFile.  Break from the while loop
			// and then store the read contents of the stream in the
			// HumdrumFile.
			break;
		}
		// (1) Does the line start with "!!!!SEGMENT"?  If so, then
		// this is either the name of the current or next file to process.
		// (1a) this is the name of the current file to process if no
		// data has yet been found,
		// (1b) or a name is being actively searched for.
		if (strncmp(templine, "!!!!SEGMENT", strlen("!!!!SEGMENT")) == 0) {
			newfilebuffer = templine;
			if (dataFoundQ) {
				// this new filename is for the next chunk to process in the
				// current file stream, not this one, so stop reading the
				// HumdrumFile content and send what has already been read back
				// out with new contents.
				break;
			}  else {
				// !!!!SEGMENT: came before any real data was read, so
				// it is most likely the name of the current file
				// (i.e., it comes at the start of the file stream and
				// is the name of the first HumdrumFile in the stream).
				PerlRegularExpression pre;
				if (pre.search(newfilebuffer,
						"^!!!!SEGMENT\\s*([+-]?\\d+)?\\s:\\s*(.*)\\s*$", "i")) {
					if (strlen(pre.getSubmatch(1)) > 0) {
						infile.setSegmentLevel(atoi(pre.getSubmatch()));
					} else {
						infile.setSegmentLevel(0);
					}
					infile.setFilename(pre.getSubmatch(2));
				}
				continue;
			}
		}
		int len = (int)strlen(templine);
		if ((len > 4) && (strncmp(templine, "!!!!", 4) == 0) &&
				(templine[4] != '!') && (dataFoundQ == 0)) {
			// This is a universal comment.  Should it be appended
			// to the list or should the currnet list be erased and
			// this record placed into the first entry?
			if (foundUniversalQ) {
				// already found a previous universal, so append.
				universals.push_back(templine);
			} else {
				// new universal comment, to delete all previous
				// universal comments and store this one.
				universals.reserve(1000);
				universals.resize(1);
				universals[0] = templine;
				foundUniversalQ = 1;
			}
			continue;
		}

		if (strncmp(templine, "*-", 2) == 0) {
			starminusFoundQ = 1;
		}

		// if before first ** in a data file or after *-, and the line
		// does not start with '!' or '*', then assume that it is a file
		// name which should be added to the file list to read.
		if (((starminusFoundQ == 1) || (starstarFoundQ == 0))
				&& (templine[0] != '*') && (templine[0] != '!')) {
			if ((templine[0] != '\0') && (templine[0] != ' ')) {
				// The file can only be added once in this manner
				// so that infinite loops are prevented.
				int found = 0;
				for (int mm=0; mm<(int)filelist.size(); mm++) {
					if (strcmp(filelist[mm].c_str(), templine) == 0) {
						found = 1;
					}
				}
				if (!found) {
					filelist.push_back(templine);
					addedFilename = 1;
				}
				continue;
			}
		}

		dataFoundQ = 1; // found something other than universal comments
		// should empty lines be treated somewhat as universal comments?

		// store the data line for later parsing into HumdrumFile record:
		buffer << templine << "\n";
	}

	if (dataFoundQ == 0) {
		// never found anything for some strange reason.
		if (addedFilename) {
			goto restarting;
		}
		return 0;
	}

	// Arriving here means that reading of the data stream is complete.
	// The string stream variable "buffer" contains the HumdrumFile
	// content, so send it to the HumdrumFile variable.  Also, prepend
	// Universal comments (demoted into Global comments) at the start
	// of the data stream (maybe allow for postpending Universal comments
	// in the future).
	stringstream contents;
	for (int i=0; i<(int)universals.size(); i++) {
		contents << &(universals[i][1]) << "\n";
	}
	buffer << ends;
	contents << buffer.str();
	infile.read(contents);
	return 1;
}


//////////////////////////////
//
// HumdrumStream::fillUrlBuffer --
//

#ifndef USING_URI

void HumdrumStream::fillUrlBuffer(stringstream& uribuffer, const char* uriname) {
	// do nothing;
}

#else

void HumdrumStream::fillUrlBuffer(stringstream& inputdata, const char* uriname) {
	inputdata.str(""); // empty any contents in buffer
	inputdata.clear(); // reset error flags in buffer

	char webaddress[123123] = {0};
	HumdrumFile::getUriToUrlMapping(webaddress, 100000, uriname);

	string hostname;
	string location;

	const char* ptr = webaddress;
	const char* filename = NULL;

	if (strncmp(webaddress, "http://", strlen("http://")) == 0) {
		// remove the "http://" portion of the webaddress
		ptr += strlen("http://");
	}

	hostname = ptr;

	for (int i=0; i<(int)hostname.size(); i++) {
		if (hostname[i] == '/') {
			hostname.resize(i);
			break;
		}
	}

	if ((filename = strchr(ptr, '/')) != NULL) {
		location = filename;
	}
	if (location.size() == 0) {
		location = "/";
	}

	char newline[3] = {0x0d, 0x0a, 0};

	stringstream request;
	request << "GET "   << location << " HTTP/1.1" << newline;
	request << "Host: " << hostname << newline;
	request << "User-Agent: HumdrumFile Downloader 1.0 ("
			  << __DATE__ << ")" << newline;
	request << "Connection: close" << newline;  // this line is necessary
	request << newline;
	request << ends;

	// cout << "HOSTNAME: " << hostname << endl;
	// cout << "LOCATION: " << location << endl;
	// cout << request.str() << endl;
	// cout << "-------------------------------------------------" << endl;

	int socket_id = open_network_socket(hostname.c_str(), 80);
	if (::write(socket_id, request.str().c_str(),
			strlen(request.str().c_str())) == -1) {
		exit(-1);
	}
	#define URI_BUFFER_SIZE (10000)
	char buffer[URI_BUFFER_SIZE];
	int message_len;
	// stringstream inputdata;
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
	// cout << header.str() << endl;
	// cout << "-------------------------------------------------" << endl;
	while (header.getline(buffer, URI_BUFFER_SIZE)) {
		int len = (int)strlen(buffer);
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
		getFixedDataSize(socket_id, datalength, inputdata, buffer,
				URI_BUFFER_SIZE);

	} else if (chunked) {
		int chunksize;
		int totalsize = 0;
		do {
			chunksize = getChunk(socket_id, inputdata, buffer, URI_BUFFER_SIZE);
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
				inputdata.write(buffer, message_len);
			} else {
				for (i=0; i<message_len; i++) {
					if (foundcontent) {
						inputdata << buffer[i];
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

	// inputdata << ends;
	//cout << "CONTENT:=======================================" << endl;
	//cout << inputdata.str();
	//cout << "===============================================" << endl;
}



//////////////////////////////
//
// HumdrumStream::prepare_address -- Store a computer name, such as
//    www.google.com into a sockaddr_in structure for later use in
//    open_network_socket.
//

void HumdrumStream::prepare_address(struct sockaddr_in *address,
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

int HumdrumStream::open_network_socket(const char *hostname,
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



//////////////////////////////
//
// getFixedDataSize -- read a know amount of data from a socket.
//

int HumdrumStream::getFixedDataSize(int socket_id, int datalength,
		stringstream& inputdata, char* buffer, int bufsize) {
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
		inputdata.write(buffer, message_len);
		readcount += message_len;
	}

	return readcount;
}



//////////////////////////////
//
//  HumdrumStream::getChunk --
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

int HumdrumStream::getChunk(int socket_id, stringstream& inputdata,
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
		cerr << "Strange error after reading newline at end of chunk size" << endl;
		exit(1);
	}

	return getFixedDataSize(socket_id, chunksize, inputdata, buffer, bufsize);
}

#endif



