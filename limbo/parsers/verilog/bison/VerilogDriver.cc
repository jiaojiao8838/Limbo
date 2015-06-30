// $Id: driver.cc 39 2008-08-03 10:07:15Z tb $
/** \file driver.cc Implementation of the example::Driver class. */

#include "VerilogDriver.h"
#include "VerilogScanner.h"

namespace VerilogParser {

Driver::Driver(VerilogDataBase& db)
    : trace_scanning(false),
      trace_parsing(false),
      m_db(db)
{
}

bool Driver::parse_stream(std::istream& in, const std::string& sname)
{
    streamname = sname;

    Scanner scanner(&in);
    scanner.set_debug(trace_scanning);
    this->lexer = &scanner;

    Parser parser(*this);
    parser.set_debug_level(trace_parsing);
    return (parser.parse() == 0);
}

bool Driver::parse_file(const std::string &filename)
{
    std::ifstream in(filename.c_str());
    if (!in.good()) return false;
    return parse_stream(in, filename);
}

bool Driver::parse_string(const std::string &input, const std::string& sname)
{
    std::istringstream iss(input);
    return parse_stream(iss, sname);
}

void Driver::error(const class location& l,
		   const std::string& m)
{
    std::cerr << l << ": " << m << std::endl;
}

void Driver::error(const std::string& m)
{
    std::cerr << m << std::endl;
}

void Driver::module_instance_cbk(std::string const& macro_name, std::string const& inst_name) 
{
	// due to the feature of LL 
	// wire_pin_cbk will be called before module_instance_cbk
	m_db.verilog_instance_cbk(macro_name, inst_name, m_vNetPin);
	// remember to clear m_vNetPin
	m_vNetPin.clear();
}
void Driver::wire_pin_cbk(std::string& net_name, std::string& pin_name, Range const& range)
{
	m_vNetPin.push_back(NetPin(net_name, pin_name, range));
}
void Driver::wire_declare_cbk(std::string const& net_name, Range const& range)
{
    m_db.verilog_net_declare_cbk(net_name, range);
}

bool read(VerilogDataBase& db, const string& verilogFile)
{
	Driver driver (db);
	//driver.trace_scanning = true;
	//driver.trace_parsing = true;

	return driver.parse_file(verilogFile);
}

} // namespace example
