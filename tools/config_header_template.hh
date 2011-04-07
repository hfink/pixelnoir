// WARNING: This file was automatically generated
// Do not edit.

#ifndef $(classname.upper() + '_' + $header_ext.upper())
#define $(classname.upper() + '_' + $header_ext.upper())

#raw
#include <string>
#end raw

$('\n'.join([line.strip() for line in $include.split('/n')]))

class $(classname)
{
    public:

#for $enum, $elems in $enums.iteritems()
    enum $(enum)
    {
#for $elem in $elems
        $(elem),
#end for
    };
#end for
    $(classname)();
    
    bool set_value(const std::string& field_name, 
                   const std::string& field_value);

    int parse_args(int argc, char** argv);

    static bool load_file(const std::string& filename, 
                          $(classname)& $(classname.lower()));
    static bool save_file(const std::string& filename, 
                          const $(classname)& $(classname.lower()));

#for $field in $fields:

    $field['type'] $(field['name'])() { return _$field['name']; }
    void set_$(field['name'])($field['type'] v) { _$field['name'] = v; }
#end for

    private:
#for $field in $fields:

#for $line in [line.strip() for line in $field['comment'].split('\n') if len(line.strip()) > 0]
    // $line
#end for
    $field['type'] _$field['name'];
#end for

};

#for $global_var in $globals
extern $(classname) $global_var;
#end for

#for $enum, $elems in $enums.iteritems()
// String converter functions for $enum
template<> inline bool from_string(const string& s, $classname::$enum& t)
{
#for $no,$elem in enumerate(elems)
    $('else 'if no > 0 else '')if (s == "$elem") { t = $classname::$elem; return true; }
#end for
    
    return false;
}

template<> inline string to_string(const $classname::$enum& t) {
    switch(t) {
#for $elem in elems
    case $classname::$elem:
        return "$elem";
#end for
    default:
        return "unknown";
    }
}

#end for
#endif
