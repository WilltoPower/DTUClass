#ifndef _DTU_GOOSE_SERVICE_H_
#define _DTU_GOOSE_SERVICE_H_

namespace DTU
{
	class dtuGooseService {
		public:
			static dtuGooseService& instance() {
				static dtuGooseService server;
				return server;
			}

		private:
			dtuGooseService() {};

		public:
			bool init();
			bool run();
			bool stop();

	};
}

#endif /* _DTU_GOOSE_SERVICE_H_ */