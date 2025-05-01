#ifdef NS3_MODULE_COMPILATION 
    error "Do not include ns3 module aggregator headers from other modules these are meant only for end user scripts." 
#endif 
#ifndef NS3_MODULE_ECC_SECURITY
    // Module headers: 
    #include <ns3/ecc-crypto.h>
    #include <ns3/ecc-key-exchange.h>
    #include <ns3/secure-socket.h>
    #include <ns3/secure-socket-helper.h>
#endif 