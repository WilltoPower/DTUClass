#ifndef DTU_DB_MANAGER_H
#define DTU_DB_MANAGER_H

#include <string>
#include <map>
#include <tuple>
#include <functional>

#include "dtubuffer.h"

#include <stdint.h>

#define ERROR_RET 0x0000

namespace DTU
{
	// 数据类型
	enum ParamType {
		PType8_t,	// uint8_t
		PType16_t,	// uint16_t
		PType32_t,	// uint32_t
		PTypeFLO_t,	// float
		PTypeSTR_t,	// std::string
		PTypeIPW_t,	// ip地址(uint32_t)
		PTypeBit_t, // bit位 bitset
		PTypeBool,	// bool
	};

	enum ParamID {
		ParamPublic,			// 公共定值
		ParamSoftPress,			// 软压板信息
		ParamGroupNo,			// 定值区号
		ParamRoutine,			// 常规保护
		ParamAutoReclose,		// 自动重合闸
		ParamAutoLocal,			// 就地馈线自动化
		ParamDistributFA,		// 智能分布式FA
		ParamSynchronousClose,	// 同期合闸
		ParamAutoSplit,			// 自动解列
		ParamSmallCurrent,		// 小电流接地
		ParamDisconnWarn,		// 线路断线告警
		ParamDriveSwitch,		// 传动开关
		ParamAutomation,		// 自动化参数
		ParamCommunication,		// 通信定值
		ParamDevice,			// 设备定值(内部定值)
		ParamAdjust,			// 整定定值
	};

	enum InfomID {
		InfomHardRemoteSignal,	// 硬件遥信
		InfomTelemetry,			// 遥测值
		InfomSelfCheck,			// 自检状态
		InfomStatusInfo,		// 状态信息
	};

	enum ReportID {
		ReportSOE,		// SOE报告
		ReportOPT,		// 操作报告
		ReportWAR,		// 告警报告
		ReportRMC,		// 遥控记录报告
		ReportProSimple,// 保护动作简报
		ReportPro,		// 保护动作报告
		ReportTransRcd, // 保护录波档案
		ReportWorkRcd,  // 业务录波档案
	};

	enum ReportType {
		ReportTypeNoFile,	// 无文件只有数据类型
		ReportTypeFile,		// 有文件只存索引类型
	};

	// 点表修改类型
	enum MapFixno {
		MAP_YX,	// 遥信
		MAP_YC,	// 遥测
		MAP_YK,	// 遥控
		MAP_YT,	// 遥调
		MAP_AU, // 自动化参数
	};

	// 每个定值的信息
	struct dtuOneParamInfo {
		dtuOneParamInfo() {}
		dtuOneParamInfo(uint16_t fixid,std::string desc,uint16_t type,int size,double min,
			double max,double step,std::string unit,std::string list,uint8_t paramid,std::string paramtype,
			uint8_t QTctrl,std::string defaultValue,int offset,bool use) 
			: fixid(fixid),desc(desc),type(type),size(size),
			min(min),max(max),step(step),unit(unit),list(list),paramid(paramid),
			paramtype(paramtype), QTctrl(QTctrl), defaultValue(defaultValue), offset(offset) ,
			use(use){}
		uint16_t	fixid = 0;
		std::string desc;
		uint16_t	type = 0;
		int			size = 0;
		double		min = 0;
		double		max = 0;
		double		step = 0;
		std::string unit;
		std::string list;
		uint8_t		paramid = 0;
		std::string paramtype;
		uint8_t		QTctrl = 0;
		std::string	defaultValue;
		int			offset = 0;
		bool use = true;
	};
	// 定值索引
	struct dtuOneParamIndex {
		dtuOneParamIndex() {}
		dtuOneParamIndex(uint16_t pid,uint32_t size,std::string desc) : pid(pid), size(size),desc(desc) {}
		uint16_t pid = 0;
		uint32_t size = 0;
		std::string desc;
		std::map<uint16_t,dtuOneParamInfo> info;// fixid || 单表信息
	};

	using dtuParamIndexTable = std::map<uint16_t, dtuOneParamIndex>;

	// 遥控表
	struct dtuRmctrlItem {
		uint16_t fixid = 0;		// 点表值
		std::string desc;		// 描述
		bool needPre = true;	// 是否需要预设
		uint16_t offset = 0;	// 多设备偏移
	};
	using dtuRmctrlTable = std::map<uint16_t, dtuRmctrlItem>;

	//////////////// 报告
	// 报告基类
	//using ReportBufferAttr = std::vector<std::tuple<uint16_t, DTU::buffer>>;
	using ReportOneBuffer = std::tuple<uint32_t, uint32_t, DTU::buffer >;
	using ReportBufferAttr = std::map<uint16_t, std::tuple<uint32_t,uint32_t, DTU::buffer>>;

	// 单报告信息
	struct dtuOneReportInfo {
		int id;
		std::string desc;
		int size = 0;
		int offset = 0;
	};

	// 报告索引
	struct dtuOneReportIndex {
		uint16_t	reportid;
		std::string desc;
		uint32_t	size;
		ReportType  type = ReportTypeNoFile;
		std::vector<std::string> paths;
		std::string table;
		uint32_t	maxCount;
		uint32_t	curNo;
		std::map<uint16_t, dtuOneReportInfo> rinfo;
	};

	using dtuReportIndexTable = std::map<uint16_t, dtuOneReportIndex>;
	using ClearAllFileCallback = std::function<void(std::vector<std::string>)>;	// 清理所有报告回调
	using ClearOneFileCallback = std::function<void(std::vector<std::string>, std::vector<std::string>)>;// 清理单个报告回调
	//////////////////////////////////////////////////////////////////////////////////
	class dtuParam {
	public:
		dtuParam(ParamID paramid);
		dtuParam(uint16_t paramid);
		~dtuParam();
	public:
		// 获取当前定值区所有定值
		bool GetCurParamValue(std::vector<std::string> &values);
		// 通过点表获取值(定值区为当前定值区)
		bool GetValueByFixid(uint16_t id, std::string& value);
		// 通过点表设置值(定值区为当前定值区)
		bool SetValueByFixid(uint16_t id, const std::string& value);
		// 通过点表预设值(定值区为预设定值区(第0定值区))
		bool PreValueByFixid(uint16_t id, const std::string& value);
		// 当前定值区src的值拷贝到dest中(依据paramID)
		bool copy(uint16_t srcgroupno, uint16_t dstgroupno);
		// 串行化
		DTU::buffer pack();
		// 反串行化
		bool unpack(const DTU::buffer& buff);
		// 获取当前定值ID号
		uint16_t GetParamID();
		// 获取当前定值最大定值区号
		int GetMaxGroup();
		// 获取当前定值 定值属性校验码buff
		std::string GetParamAttributeBuff();
		// 当前定值所包含的定值数
		int count();
	private:
		// 友元函数
		friend class DBManager;
		using AllParamValue = std::map<uint16_t, std::vector<std::string>>;
		void GetAllParamValue(AllParamValue& param);
		void SetAllParamValue(AllParamValue& param);
	private:
		void* params = nullptr;
		uint16_t _paramid = 0xFFFF;
		bool isChange = false;// 是否可以切换定值区
	};

	struct dtuInfoOneItem {
		uint16_t fixid = 0;
		std::string desc;
		uint16_t type = 0;
		uint16_t size = 0;
		uint16_t infoid = 0;
		uint16_t qtctrl = 0;
		std::string defaul;
		int offset = 0;
	};

	struct dtuInfoIndex {
		uint16_t iid = 0;
		std::string desc;
		uint16_t size = 0;
		std::map<uint16_t, dtuInfoOneItem> info;
	};

	using dtuInfoTable = std::map<uint16_t, dtuInfoIndex>;

	using dtuParamCheckIndex = std::map<uint16_t, std::tuple<uint16_t, std::string, float>>;

	// SOE
	struct dtuOneSOEitem {
		uint16_t fixno = 0;
		uint16_t innerno = 0;
		std::string desc;
		std::string adddesc;
		bool use = true;
	};
	// first为内部点表值
	using dtuSOEIndex = std::map<uint16_t, dtuOneSOEitem>;//SOE索引表

	struct dtuOneCOSitem {
		uint16_t fixno = 0;
		uint16_t innerno = 0;
		std::string desc;
		std::string adddesc;
		bool use = true;
	};
	// first为内部点表值
	using dtuCOSIndex = std::map<uint16_t, dtuOneCOSitem>;//COS索引表

	// 遥测,遥信表
	struct dtuOneYXItem {
		uint16_t fixno = 0;
		uint16_t offset = 0;
		uint16_t size = 1;
		std::string desc;
		bool use = true;
		uint16_t devno = 0;
	};
	enum YX_TYPE{
		YX_SYX,
		YX_HYX,
		YX_ALL,
	};
	using dtuYXIndex = std::map<uint16_t, dtuOneYXItem>;	//遥信量

	struct dtuOneRMCitem {
		uint16_t addr = 0;			// 外部点表
		std::string desc;			// 描述
		uint16_t inoperate1 = 0;	// 内部操作1点表
		uint16_t inoperate2 = 0;	// 内部操作2点表
		bool use = true;
	};
	// first为外部点表
	using dtuRmctrlIndex = std::map<uint16_t, dtuOneRMCitem>;//遥控索引表
	//////////////////////////////////////////////////////////////////////////////////
	enum UPOWER {
		ADMINISTRATORS, /* 管理员 */
		USER,			/* 普通用户 */
		VISITOR,		/* 游客 */
	};
	struct oneUserInfo {
		std::string name;		/* 名字 */
		std::string password;	/* 密码 */
		UPOWER power = VISITOR;	/* 权限 */
		std::string word;		/* 口令 */
	};

	using UserInfo = std::vector<oneUserInfo>;

	struct oneFIXNOMAP {
		int tableno;
		int begin = 0x00;
		int end = 0x00;
		std::string desc;
		int offset = 5;
	};

	using FIXNOMAP = std::vector<oneFIXNOMAP>;
	//////////////////////////////////////////////////////////////////////////////////
	class DBManager {
	private:
		DBManager();
		~DBManager();
	public:
		static DBManager& instance()
		{
			static DBManager ins;
			return ins;
		}
	using DSPSendFunc = std::function<int(uint16_t, const DTU::buffer&, uint16_t)>;
	public:
		// 初始化
		bool init(std::string dbPath);
	
	public: 
		/*--------------------------------------- 定值相关 ----------------------------------------------*/
		// 从文件存储中读取定值
		bool readParamValue(dtuParam &param);
		// 保存定值到文件存储中
		bool saveParamValue(dtuParam &param);

		// 通过点表号获取ParamID号
		uint16_t GetParamIDByFixid(uint16_t fixid);
		// 预设定值
		bool setPreParamValue(uint16_t fixid, std::string value);
		bool setPreParamValue(uint16_t fixid, DTU::buffer value);
		// 确认预设
		bool confirmPreParamValue(DSPSendFunc func,uint16_t reboot = 1);
		// 取消预设
		bool cancelPreParamValue();
		// 定值区拷贝(只在数据库之间拷贝,不会下发)
		bool copyAllParamGroup(uint16_t src, uint16_t dest);
		// 下发当前定值区所有定值
		bool setAllParamToDSP(DSPSendFunc sendfunc,uint16_t reboot = 1);

		// 获取所有定值的信息
		const dtuParamIndexTable& GetParamInfo();
		// 通过ParamID获取该定值的所有信息
		const dtuOneParamIndex& GetParamInfoByID(uint16_t paramid);
		// 通过点表号获取该定值的信息
		const dtuOneParamInfo& GetOneParamInfoByFix(uint16_t fixid);
		// 通过DESC查找定值表点表(该函数可能会因查询的字符编码导致查询不出来,只有UTF-8才能查询出来)
		uint16_t GetParamFixidByDesc(std::string desc);
		// 获取单个定值表元素全部信息
		dtuOneParamInfo GetOneParamItemByFixid(uint16_t fixid);

	public:
		/*-------------------------------------- 定值区相关 ---------------------------------------------*/
		// 获取编辑区号
		uint16_t GetEditGroup();
		// 获取当前定值区号
		uint16_t GetCurrGroup();
		// 设置编辑区号
		bool SetEditGroup(uint16_t groupno);
		// 设置当前定值区号
		bool SetCurrGroup(uint16_t groupno);
		// 获取定值区最大组号
		uint16_t GetMaxiGroup();
		// 切换定值区
		bool ChangeCurGroup(uint16_t targetgroup, DSPSendFunc callback, uint16_t reboot = 1);

	public:
		/*--------------------------------------- 报告相关 ----------------------------------------------*/

		// 获取所有报告信息
		const dtuReportIndexTable& GetReportInfo();
		// 通过ReportID获取单个报告的信息
		const dtuOneReportIndex& GetReportInfoByID(uint16_t reportid);
		// 根据ReportID获取当前最新一条报告序号
		uint32_t GetCurReportNoByID(uint16_t reportid);
		// 添加报告
		bool addReport(uint16_t reportid, uint32_t time_m, uint32_t time_ms, const DTU::buffer& data);
		// 按照索引读取单篇报告(索引从1开始)
		bool readReportByIndex(uint16_t reportid, uint32_t index, DTU::buffer& data);
		// 范围读取报告(min,max的值都能读取的到)
		bool readReport(uint16_t reportid, int min,int max, ReportBufferAttr &data);
		// 清理报告
		bool clearReport(uint16_t reportid);
		// 设置报告存储前置路径(需要在初始化后立马设置,在DSP启动前设置)
		void setReportFilePathPre(std::string path);
		// 设置清理单文件回调(需要在初始化后立马设置,在DSP启动前设置)
		void setReportClearOneCallback(ClearOneFileCallback callback);
		// 设置清理所有文件回调(需要在初始化后立马设置,在DSP启动前设置)
		void setReportClearAllCallback(ClearAllFileCallback callback);

	public:
		/*--------------------------------------- 命令相关 ----------------------------------------------*/

		// 通过ParamID获取读命令
		uint16_t Get_R_CMD_By_ParamID(ParamID pid);
		// 通过ParamID获取读命令
		uint16_t Get_R_CMD_By_ParamID(uint16_t pid);
		// 通过ParamID获取写命令
		uint16_t Get_W_CMD_By_ParamID(ParamID pid);
		// 通过ParamID获取写命令
		uint16_t Get_W_CMD_By_ParamID(uint16_t pid);
		// 通过命令获取ParamID号
		uint16_t Get_ParamID_By_CMD(uint16_t cmd);
		// 通过读命令获取ReportID
		uint16_t Get_ReportID_By_CMD(uint16_t cmd);

	public:
		/*--------------------------------------- 点表相关 ----------------------------------------------*/
		// 获取遥控表
		const dtuRmctrlTable& GetRmctrlTable();

	public:
		/*-------------------------------------- 信息表相关 ---------------------------------------------*/
		// 获取全部信息表
		const dtuInfoTable& GetInfomationTable();
		// 根据infoID获取信息表
		const dtuInfoIndex& GetInfomationTableByIndex(uint16_t infoID);
		// 通过DESC查找信息表点表(该函数可能会因查询的字符编码导致查询不出来,只有UTF-8才能查询出来)
		uint16_t GetInfomFixidByDesc(std::string desc);
		// 获取单个信息表元素全部信息
		dtuInfoOneItem GetOneInfoItemByFixid(uint16_t fixid);
	
	public:
		/*------------------------------------- 定值属性相关 --------------------------------------------*/

		// 获取定值属性相关表
		dtuParamCheckIndex GetParamCheckTable();
		// 定值属性校验相关
		std::string GetParamAttributeCheckBuff();
		// 定值校验相关
		std::string GetParamCheckBuff();

	public:
		/*------------------------------------- 点表映射相关 --------------------------------------------*/

		// 遥控 遥调 遥测 遥信

		// 获取所有遥信索引表
		const dtuSOEIndex& GetSOEIndex();
		// 获取所有遥测索引表
		const dtuCOSIndex& GetCOSIndex();
		// 获取所有遥控索引表
		const dtuRmctrlIndex& GetRMCIndex();
		// 检查点表是否可以被修改目的值(外部点表值)
		bool CheckFixnoReady(MapFixno maptype, uint16_t outfixno);
		// 更新点表值(外部点表值)
		bool ModifyFixno(MapFixno maptype,uint16_t older,uint16_t newer);

		// 按类获取遥信索引表
		dtuYXIndex GetYXIndex(YX_TYPE type);

		// SOE映射查找(通过内部点表查找外部映射点表)
		uint16_t GetSOEMapFixidByinID(uint16_t infixid);
		// COS映射查找(通过内部点表查找外部映射点表)
		uint16_t GetCOSMapFixidByinID(uint16_t infixid);
		// 遥控映射查找(通过映射的点表查找内部操作点表) exec:在分闸操作中是否执行
		uint16_t GetRMCMapFixidByoutID(uint16_t outfixid, int opewhat, bool& exec);
		// 遥控映射查找(内部操作点表查找外部映射点表)
		uint16_t GetRMCMapFixidByinID(uint16_t infixid);

	private:
		// 定值地址 通过外部点表值计算内部点表值 (此函数并未检查是否越界)
		uint16_t GetParamMapFixidByoutID(uint16_t outfixid);
		// 定值地址 通过内部点表值计算外部点表值 (此函数并未检查是否越界)
		uint16_t GetParamMapFixidByinID(uint16_t infixid, bool isfrombay, int baydennoo);

		// 遥测通过内部地址映射外部地址 (此函数并未检查是否越界)
		uint16_t GetYCMapFixidByinID(uint16_t infixid, bool isfrombay, int baydennoo);
		// 遥测通过外部地址映射内部地址 (此函数并未检查是否越界)
		uint16_t GetYCMapFixidByoutID(uint16_t outfixid);

		// 自动化参数外部定值计算内部点表值 (此函数并未检查是否越界)
		uint16_t GetAutoMapFixidByinID(uint16_t infixid, bool isfrombay, int baydennoo);
		// 自动化参数外部定值计算内部点表值 (此函数并未检查是否越界)
		uint16_t GetAutoMapFixidByoutID(uint16_t outfixid);

		// 遥信点表外部映射  (此函数并未检查是否越界)
		uint16_t GetYXMapFixidByinID(uint16_t infixid, bool isfrombay, int baydennoo);
		// 遥信点表内部映射  (此函数并未检查是否越界)
		uint16_t GetYXMapFixidByoutID(uint16_t outfixid);

		// 通过内部点表映射外部点表
		uint16_t GetYKMapFixidByinID(uint16_t infixid, bool isfrombay, int baydennoo);
		// 通过外部点表映射内部点表
		uint16_t GetYKMapFixidByoutID(uint16_t outfixid);

	public:
		// 外部点表映射内部点表  (此函数并未检查是否越界)
		uint16_t FixidMapOuttoin(MapFixno type, uint16_t outfixid);
		// 
		uint16_t FixidMapOuttoin(uint16_t outfixid);
		// 内部点表映射外部点表  (此函数并未检查是否越界)
		uint16_t FixidMapIntoout(MapFixno type, uint16_t infixid, bool isfrombay = false, int baydennoo = 0);

		void SetDevNo(int devno);
		bool isDevNoSet();


		struct FIXIDINFO {
			bool ok = false;
			int devno = 0;
			int tableno;
		};

		// 判断规约中的点表来自于哪一个点表和设备
		FIXIDINFO whereFixFrom(uint16_t outfixid);
		// 测试点表是否属于本设备
		bool testFixidBelongCurDevice(uint16_t outfixid);
		// 硬件号是否需要主动上送主站(测试遥信和遥测)
		bool isDevIDneedToMaster(MapFixno type, uint16_t devid);

	private:
		int devno = 0;
		bool isSetDevno = false;
		int paramsize = 0;
		int CalculateRuleParamSize();
		FIXNOMAP StaticMAPtable;

	public:
		/*------------------------------------- 用户管理相关 --------------------------------------------*/
		enum UserError {
			NO_USER,
			PW_UNCORRECT,
			PW_CORRECT,
		};
		// 用户名是否正确
		bool isPasswordCorrect(const std::string &name, const std::string &password, UserError &flag);
		// 获取用户权限
		UPOWER GetUserPower(const std::string &name);
		// 获取管理员口令
		std::string GetRootKey();
		// 用户是否存在
		bool isUserExist(const std::string &name);

		// TEST API
		void TESTAPI();
	};
}

#endif // DTU_DB_MANAGER_H