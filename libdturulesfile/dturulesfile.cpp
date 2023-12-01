#include "dturulesfile.h"
#include "dtuexvrecorder.h"
#include "dtufixrecorder.h"
#include "dtulogrecorder.h"
#include "dtusoerecorder.h"
#include "dtucorecorder.h"
#include "dtuelecenergyfrz.h"
#include "dtuelecenergyfro.h"

using namespace DTU;
void DRULESFILE::add_exv_data(buffer& data)
{
    DExvRcd::instance().add_exv_data(data);
}
void DRULESFILE::start_fix_service()
{
    DFixRcd::instance().form_fix_rcd();
}
void DRULESFILE::stop_fix_service()
{
    DFixRcd::instance().stop_form_fix_rcd();
}
void DRULESFILE::add_log_data(buffer& data)
{
    DLogRcd::instance().add_log_data(data);
}
void DRULESFILE::get_fix_dir(buffer& data)
{
    DFixRcd::instance().get_fix_dir(data);
}
void DRULESFILE::get_exv_dir(buffer& data)
{
    DExvRcd::instance().get_exv_dir(data);
}
void DRULESFILE::form_log_file(){
    DLogRcd::instance().form_log_file();
}
void DRULESFILE::form_soe_file(){
    DSoeRcd::instance().form_soe_file();
}
void DRULESFILE::add_co_file(uint16_t fix,int opt,int status)
{
    DCoRcd::instance().add_co_file(fix,opt,status);
}
void DRULESFILE::form_co_file(){
    DCoRcd::instance().form_co_file();
}
void DRULESFILE::set_mode(uint16_t addr, uint16_t mode)
{
    DExvRcd::instance().set_mode(addr,mode);
    DFixRcd::instance().set_mode(addr,mode);
    DLogRcd::instance().set_mode(addr,mode);
    DExvRcd::instance().set_mode(addr,mode);
    DFroRcd::instance().set_mode(addr,mode);
    DFrzRcd::instance().set_mode(addr,mode);
}


void DRULESFILE::start_frz_service()
{
    DFrzRcd::instance().form_frz_rcd();
}

void DRULESFILE::stop_frz_service()
{
    DFrzRcd::instance().stop_form_frz_rcd();
}

void DRULESFILE::add_frz_rcd(buffer& data)
{
    DFrzRcd::instance().add_frz_rcd(data);
}

void DRULESFILE::add_fro_data(buffer& data)
{
    DFroRcd::instance().add_fro_rcd(data);
}
