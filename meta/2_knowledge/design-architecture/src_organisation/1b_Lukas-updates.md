removed handlers/
integrated HttpMethods within http/


src/config/frontend/
&
inc/config/Frontend.hpp

->
src/config/ConfigFrontend/
&
inc/config/ConfigFrontend.hpp


reintegration of subdirectory src/Config/Config.cpp
from src/Config.cpp


WebServ/
extracted from core/

now:
  src/core/Server & src/core/signal.cpp
  separate from: src/WebServ/

  inc/WebServ.hpp
  separate from: inc/core/



removal of tmp/
reintegration of temporary:

  HttpParser.hpp
  into inc/http/

  and

  HttpParser.cpp
  into src/http/HttpResponseFrontend/
