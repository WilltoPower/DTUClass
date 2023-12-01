/*
1，负责进行初始化
*/

#pragma once

namespace DTUTool {
	class dtuToolinit {
		public:
			static dtuToolinit &instance() {
				static dtuToolinit ins;
				return ins;
			}

		private:
			dtuToolinit();
		
		public:
			bool init();
	};
}