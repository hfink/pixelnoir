// WARNING: This file was automatically generated
// Do not edit.

$('#include') "$classname.$header_ext"

#raw
#include <iostream>
#include <fstream>
#include <boost/regex.hpp>
#end raw

#for $global_var in $globals
$classname $global_var;
#end for

$classname::$(classname)() 
{
#for $no,$field in enumerate($fields)
    set_value("$field['name']", "$field['default']");
#end for
}

bool $classname::set_value(const std::string& field_name,
                           const std::string& field_value)
{    
#for $no, $field in enumerate($fields)
    $('} else 'if no > 0 else '')if (field_name == "$(field['name'])") { 
        return from_string(field_value, _$field['name']);
#end for
    }

    return false;
}

int $classname::parse_args(int argc, char** argv)
{
    const boost::regex pattern("--(\\w+)=(.+)$");
    boost::match_results<std::string::const_iterator> match;
    
    int other = 0;

    for (int i = 0; i < argc; ++i) {
		string arg(argv[i]);
        if (boost::regex_match(arg, match, pattern)) {
            bool success = set_value(match[1], match[2]);

            if (!success) {
                cerr << "Failed to set " << match[1] 
                     << " to " << match[2] << endl;
            }
        } else {
            argv[other] = argv[i];
            other++;
        }
    }

    return other;
}

bool $classname::save_file(const std::string& filename,
                           const $(classname)& config)
{
    std::ofstream os(filename.c_str());

    if (!os) {
        std::cerr << "Could not open file " << filename << " for writing." << std::endl;
        return false;
    }

#for $field in $fields
#for $line in [line.strip() for line in $field['comment'].split('\n') if len(line.strip()) > 0]
    os << "// $line" << std::endl;
#end for
    os << "$field['name'] = " << to_string(config._$(field['name'])) << std::endl;
    os << std::endl;

#end for
    return true;
}

bool $classname::load_file(const std::string& filename,
                           $(classname)& config)
{
    const int MAX_LENGTH = 1000;
    char line[MAX_LENGTH];

    const boost::regex pattern("\\s*(\\w+)\\s+=\\s+(.+)\\s*$");
    boost::match_results<std::string::const_iterator> match;

    std::ifstream is(filename.c_str());

    if (!is) {
        return false;
    }

    while(is.getline(line, MAX_LENGTH)) {
        std::string strline(line);
        if (boost::regex_match(strline, match, pattern)) {
            bool success = config.set_value(match[1], match[2]);

            if (!success) {
                return false;
            }
        }
    }

    return true;
}
