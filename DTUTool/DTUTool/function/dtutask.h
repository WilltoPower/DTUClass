#pragma once

#include <QString>
#include <QFile>
#include <QString>
#include "dtuprotocol.h"
#include "dtucommon.h"
#include "dtudbmanager.h"
#include "dtusystemconfig.h"

// д��ֵ
int execute_write_setting(uint16_t cmd, const DTU::buffer& data, uint16_t group);
// ��ȡ��ֵ
int execute_read_setting(uint16_t cmd, DTU::buffer& result, uint16_t group);
// ��ȡ��Ϣ
int execute_query_data(uint16_t cmd, DTU::buffer& result);
// д�����
int execute_write_data(uint16_t cmd, const DTU::buffer& result);
// �����ļ�
int execute_get_file(const std::string& fileName, DTU::buffer& result);
// ���ļ��б�
int execute_get_dir(const std::string& dirName, uint64_t begin, uint64_t end, FILELIST& result);
// ң��
int execute_rmctrl(uint16_t fix, uint16_t operate, int delay, int from);
// ��ȡ����
int execute_get_report(uint16_t id, int min, int max, DTU::ReportBufferAttr& result);
int execute_get_reportno(uint16_t id, uint32_t &reportno);
// �������
int execute_clear_report(uint16_t id);
// �ϴ��ļ�(���ļ��ϴ�)
int execute_set_file(const std::string& srcName, const std::string& destName);
// �ϴ��ļ�(ֱ�Ӵ��ļ�����)
int execute_set_file(const std::string& destNameName, const DTU::buffer& fileContent);
// �л���ֵ��
int execute_change_group(uint16_t dst);
// ��ȡ��ֵ����Ϣ
int execute_read_group(uint32_t& curgroup, uint32_t& maxgroup);
// ִ����������
int execute_updateprogram(uint16_t tag);
// ��ȡ��������
int execute_get_disksuage(Disk_info &usage, uint16_t tag);
// ��ȡGOOSE����
int execute_read_goose_cfg(DTUCFG::DSYSCFG::GooseCFG &gcfg);
// д��GOOSE����
int execute_save_goose_cfg(DTUCFG::DSYSCFG::GooseCFG &gcfg);
// ��������״̬
void set_arm_connect_state(bool state);
// �����Ƿ��Ѿ�����
bool execute_test_arm_connect();
// ��ȡARM����·��
int execute_get_filepath(std::string &Path);
// ������Ƿ�����޸�
bool execute_fixno_check(DTU::MapFixno type, uint16_t fixno);
// �޸ĵ��
bool execute_fixno_modify(DTU::MapFixno type, uint16_t older, uint16_t newer);

// ��װ�ô�����ļ�(10M����)
int execute_set_file_plus(const std::string& srcName, const std::string& destName);
// ��װ�ô����ļ���PC(10M����)
int execute_get_file_plus(const std::string& srcName, const std::string& destName);