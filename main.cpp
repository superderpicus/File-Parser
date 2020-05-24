// file parsing tool - by taters#0001
// created this tool to encrypt and decrypt files easily
// for use in modding monster hunter world
#include "Blowfish.h"
#include "Keys.h"
#include <iostream>
#include "string.h"
#include <sys/stat.h>
#include <conio.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>

// our own any key to contine function, because fuck system("pause");
void pause()
{
    std::cout << "Press any key to continue...";
    _getch();
    //std::cin.get(); // annoying, only works with enter
    return;
};

// easier for me to use enum
// to handle encrypt/decrypt state
enum MODE
{
    ENCRYPT,
    DECRYPT
};

// file type enum for detecting
// input in the args
enum FILE_TYPE
{
    FILE_,
    DIR,
    OTHER,
    ERROR
};

// string version of the enum
std::string FILE_TYPE_STRING[4] =
{
    "File",
    "Folder",
    "Other",
    "Error"
};

// Customizable variables
MODE mode = ENCRYPT;
const bool dev = false;

// file prefixes for output
int fCount = 0;
const std::string en_prefix = ""; // en_
const std::string de_prefix = "decrypted\\";
std::string current_dir; // current directory variable
std::vector<std::string> folder_stack;
std::string getCurrentFolderStack()
{
    std::string s;
    for (auto& v : folder_stack)
    {
        s = s + v;
    }
    return s;
}

// function to check if a file is supported,
// and return the key if it is supported
const char* supportedFileType(const char* fileExt)
{
    std::map<const char*, const char*>::iterator itr; // iterator for the map of keys in 'Keys.h'
    bool found = false; // variable to flag if key is found later on
    const char* key = ""; // key string to set and return later, if found in the map
    for (itr = supportedFileTypes.begin(); itr != supportedFileTypes.end(); ++itr)
    {
        // have to make strings here for some reason...
        // comparing 2x const char* always failed check,
        // even if they were exactly the same
        std::string ext = fileExt; // ext = extension of file we are checking
        std::string it = itr->first; // it = extension from iterator iterating the map of keys

        // comparison
        if (ext == it)
        {
            // the 2 extensions are equal,
            // set found to true and assign
            // the key string to the key
            // associated in the map
            found = true;
            key = itr->second;
        }
    };
    // if found then return key else return "error" end
    return found ? key : "error";
};

// arg class is a class made to parse
// 1 file per object
class arg
{
public:
    // passed arg = the file path passed from int main(int argc, char** argv)
    // i = arg number, used for debugging purposes
    arg(const char* passedArg, int i)
    {
        int result = init(passedArg, i); // init result
        switch(result)
        {
        case 0: // no problem
            // init successful, file exists and is valid
            std::cout << "\t[File " << i << "] The file: [" << fName << fExt << "] is supported\n";
            // actually parse the file now
            parseFile(i);
            break;
        case 1: // file not supported
            std::cout << "\t[File " << i << "] The file: [" << fName << fExt << "] is not supported\n";
            break;
        case 2: // setOutput returned false
            break;
        case 3: // eof
            break;
        default:
            break;
        };
    };
    ~arg(){ /*delete fPath; delete fExt; delete fName;*/ };
    std::string &getPath(){return this->fPath;} // get path var
    std::string &getExt(){return this->fExt;} // get ext var
    std::string &getName(){return this->fName;} // get name var
    std::string getKey() {return (std::string)this->key;} // get key var
    bool setOutput(const char* path); // set output file
    bool parseFile(int i); // parse bytes
private:
    int init(const char* passedArg, int i); // initialize
    std::string fPath; // file path
    std::string fExt; // file extension
    std::string str; // str var to hold the name ... breaks otherwise ?
    std::string fName; // name without ext
    const char* key; // 56 letter encryption key
    std::string output; // output path string
};

// remove overlapping characters
std::string subtractString(std::string a, std::string b)
{
    std::string newString;
    int aLen = a.length();
    int bLen = b.length();
    int diff = bLen - aLen;
    if (diff <= 0) return "";

    for (int i = 0; i < aLen; i++)
    {
        if (a[i] != b[i])
            newString += b[i];
    }
    newString += b.substr(aLen, diff);
    return newString;
}

std::string getNewOutput(std::string p)
{
    return subtractString(current_dir, p);
}

// convert string to unsigned char pointer
// because apparently you need that ...
unsigned char* newUChar(std::string str)
{
    unsigned char* k = new unsigned char[str.length() + 1]; // stuff
    strcpy((char*)k, str.c_str()); // happening
    return k; // return new unsigned char pointer :)
}

// readBuffer is a function that takes a file
// path, and size of the file, and reads it
// into a char array for later use.
char* readBuffer(const char* path, size_t& s)
{
    std::ifstream t(path, std::ios::binary | std::ios::ate); // open file 'path' at the end, for reading the size
    size_t size = t.tellg(); // get size of the file opened
    char* buffer = new char[size + 1]; // create buffer with size+1, to allow terminating char
    std::fill_n( buffer, size+1, '\0' ); // fill buffer with '\0'
    t.seekg(0); // seek the beginning
    t.read(buffer, size); // read file into buffer
    t.close(); // close file
    s = size; // set size to s
    return buffer; // return buffer
}

// writeOutput is a function that literally just writes
// the size from arg3 of output (arg1) to path (arg2)
void writeOutput(char* output, const char* path, size_t& size)
{
    std::ofstream f(path, std::ios::binary | std::ios::trunc); // open file 'path', with truncate option
	f.write(output, size); // simply write the data from 'output' to the file
	f.close(); // close
}

// BSwap is a function that is specially made to
// reverse the order of every 4 bytes because
// fuck capcom? or something
unsigned char* BSwap(unsigned char* data, size_t& size)
{
    unsigned char* result = new unsigned char[size+1];  // new buffer for flipped data
    result[size+1] = '\0'; // terminating character

    // loop through every 4 bytes
    for (size_t i = 0; i < size; i += 4)
    {
        // flip bytes
        result[i]     = data[i + 3];
        result[i + 1] = data[i + 2];
        result[i + 2] = data[i + 1];
        result[i + 3] = data[i];
    }

    return result; // return new buffer
}

// function to check if file already exists
inline bool fileExists (const std::string& name)
{
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

FILE_TYPE checkFileType(const char* path)
{
    // https://stackoverflow.com/questions/146924/how-can-i-tell-if-a-given-path-is-a-directory-or-a-file-c-c
    struct stat s;
    if( stat(path,&s) == 0 )
    {
        if( s.st_mode & S_IFDIR )
            return FILE_TYPE::DIR;
        else if( s.st_mode & S_IFREG )
            return FILE_TYPE::FILE_;
        else
            return FILE_TYPE::OTHER;
    }
    return FILE_TYPE::ERROR;
}

std::string toString(int i)
{
    std::stringstream ss;
    ss << i;
    return ss.str();
}

// parseFile function simply uses everything
// that I built beforehand. Loads file into
// buffer, flips bytes, creates blowfish object,
// encrypts/decrypts, flips bytes, and writes to file
bool arg::parseFile(int i)
{
    size_t size = 0; // declare size here so its available to all the functions
	char* buffer = readBuffer(this->fPath.c_str(), size); // read buffer from the file
	unsigned char* data = BSwap((unsigned char*)buffer, size); // new buffer from the byte swapped buffer
	delete buffer; // remove buffer from memory
	char* output = new char[size + 1]; // output buffer that will be written to file
    std::fill_n( output, size+1, '\0' ); // fill output with terminating character

    if(dev) // debug message
        std::cout << "File [" << this->fName << this->fExt << "] is (" << size << ") BYTES\n";

    unsigned char* key = newUChar(this->key);

    // create blowfish object, and pass our key we grabbed earlier
    CBlowFish bf(key, 56);

    // main blowfish functions are inside try block
    try
    {
        bf.ResetChain(); // reset in case of loop
        std::string complStr = "[File " + toString(i) + "] "; // string for use later, in the message
        // check mode for which operation to complete
        if (mode == ENCRYPT)
        {
            // encryption mode
            bf.Encrypt(data, (unsigned char*)output, size, CBlowFish::ECB);
            complStr += "Encryption complete: [";
        }
        else if(mode == DECRYPT)
        {
            // decryption mode
            bf.Decrypt(data, (unsigned char*)output, size, CBlowFish::ECB);
            complStr += "Decryption complete: [";
        }
        // now swap the bytes again and send it to new buffer variable
        unsigned char* oData = BSwap((unsigned char*)output, size);
        delete output; // delete output from memory

        if (fileExists(this->output))
            std::cout << "\t[File " << i << "] Warning: Overwriting file [" << this->output << "]\n";
        writeOutput((char*)oData, this->output.c_str(), size);

       // delete oData; // delete oData from memory but DONT because it causes errors 
        std::cout << complStr << this->output << "]\n"; // output message for user to know
        return true; // success
    }
    catch(std::exception& e)
    {
        // exception occured (reeeeee)
        std::cout << e.what() << "\n";
    };
    return false; // if you made it here, there was an error
}

// function simply sets the path
// for the output file
bool arg::setOutput(const char* path)
{
    std::string n = this->fName; // get filename
    int pos_en = fName.find("en_"); // pos = position in string or -1 if not found
    if (pos_en != -1) // check pos is valid and not -1
    {
        // it is valid, which means de_ was found.
        // we have to remove it from the path
        n = this->fName.substr(3);
        if (mode == ENCRYPT)
        {
            std::cout << "Error: Attempting to encrypt an encrypted file...\n";
            return false;
        }
    }
    int pos_de = fName.find("de_"); // reuse pos and check for en_ this time
    if (pos_de != -1)
    {
        // found again, throw error because you shouldn't be
        // encrypting this file again
        n = this->fName.substr(3);
        if (mode == DECRYPT) 
        {
            std::cout << "Error: Attempting to decrypt a decrypted file...\n";
            return false;
        }
    }
    // now assign output to the new prefix
    std::string pref = mode == DECRYPT ? "decrypted\\" : "encrypted\\";
    std::string _temp = pref + getNewOutput(path);

    const char* fileName;
    fileName = strrchr(path, '\\'); // reverse char search
    if (fileName == NULL) // no backslash was found
        fileName = path; // set new string to the arg passed
    else // backslash was found
        fileName = fileName + 1; // increment string, to remove the backslash from the result

    int len = strlen(fileName);

    _temp = _temp.substr(0, _temp.length() - len);

    std::filesystem::create_directories(_temp + "\\");
    this->output = _temp + fileName; // new output path :)
    //std::cout << "creating:" << _temp << "\\\n";
    if (dev) // debug message
        std::cout << "OUTPUT FILE: " << this->output << "\n";
    return true; // success
};

// main function to set up all the parts for parsing
int arg::init(const char* passedArg, int i)
{
    // have to remove the big path from files
        const char* fileName;
        fileName = strrchr(passedArg, '\\'); // reverse char search
        if (fileName == NULL) // no backslash was found
            fileName = passedArg; // set new string to the arg passed
        else // backslash was found
            fileName = fileName + 1; // increment string, to remove the backslash from the result

        std::string m = mode == ENCRYPT ? "encrypt" : "decrypt"; // for ease of use and avoiding and if statement
        std::cout << "\t[File " << i << "] Attempting to " << m << " file: [" << fileName << "]...\n";

        if (dev) // debug message
            std::cout << "Argument: " << i << "\n\tPath:\t[" << passedArg << "]\n\tName:\t[" << fileName << "]\n"; // debug output

        // now look for file extension
        const char* fileExt;
        fileExt = strchr(fileName, '.');
        if (fileExt == NULL) // no period was found, is it even a file lol
        {
            std::cout << "\tError: Invalid file passed: [" << fileName << "]\n"; // error it out
        }
        else // period was found
            fileExt = fileExt; //+ 1; // increment string, to remove the period from the result

        // fileExt exists and is valid
        if (fileExt != NULL)
        {
            std::string full = (std::string)fileName; // convert fileName to string for substr usage
            full = full.substr(0, strlen(fileName) - strlen(fileExt) ); // convert full to just the file, not the extension
            const char* f = full.c_str();
            if (dev)
            {
                std::cout << "\tExt:\t[" << fileExt << "]\n";
                std::cout << "\tRName:\t[" << f << "]\n";
            }
            // pass values into the variables of 'arg' class
            this->fExt = fileExt;
            this->str = full; // have to set this var, because just setting fName to full.c_str() makes it go out of scope
            this->fPath = passedArg;
            this->fName = this->str.c_str();

            if (!this->setOutput(passedArg)) // if setOutput errored out, abort
                return 2;

            // use supportedFielType checker, it also returns key if it is supported
            const char* res = supportedFileType(fileExt);
            if ((std::string)res == "error")
            {
                if (dev) // dev message
                    std::cout << "Error: file extension: [" << fileExt << "] is NOT supported\n";
                return 1;
            }
            else
            {
                if (dev) // dev message
                    std::cout << "Fetched key: " << res << "\n";
                this->key = res; // assign key variable to the result we got earlier
                return 0; // success
            }
        }
        return 3;
};

std::vector<std::string> getFilesInDirectory(const char* path)
{
   std::vector<std::string> s;
   for (const auto & entry : std::filesystem::directory_iterator(path))
        s.push_back(entry.path().string());
   return s;
}

void _parse(const char* path, int i, FILE_TYPE type)
{
    if (type == FILE_)
    {
        fCount++;
        //std::cout << fCount << " : File Path : " << path << "\n";
        arg newArg(path, fCount);
    }
    else if (type == DIR)
    {
        std::vector<std::string> dir = getFilesInDirectory(path);
        for (auto& s : dir)
        {
            FILE_TYPE t = checkFileType(s.data()); // get file type
            _parse(s.data(), fCount, t);
        }
    }
}

// parse a folder input if it exists
void parseInput(const char* path, int i)
{
    FILE_TYPE t = checkFileType(path); // get file type
    if (dev) // debug message
        std::cout << "Input Type : " << FILE_TYPE_STRING[t] << "\n";
    _parse(path, fCount, t);
};

// int main function
int main(int argc, char** argv)
{
    current_dir = std::filesystem::current_path().string() + "\\"; // current path of exe

    //std::cout << "current_dir var: " << current_dir << "\n";
    fCount = 0;

    std::cout << "<Encryption/Decryption Tool by taters>\n";

    // debug info
    if (dev)
    {
        std::cout << "Arg Count: " << argc - 1 << "\n";
    }
    // arg0 = this executable
    if (argc - 1 == 0)
    {
        std::cout << "Error: No Files Passed\n";
        pause(); // any key to continue
        return 1;
    }

    // iterate passed arguments to program and push a file for each one
    for (int i = 1; i < argc; i++)
    {
        parseInput(argv[i], fCount); // call main parse function on args
    };

    pause(); // any key to continue
    return 0; // success
};