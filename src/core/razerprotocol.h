#pragma once
#include "common.h"
class RazerProtocol
{
public:
    static Json models();
    static std::vector<Bytes> lightingReports(const Json& steps, const Color& color, int brightness);
    static Bytes dpiReport(const Json& model, int dpi);
    static Bytes report(int commandClass, int command, int size, int transaction = 0);
    static void checksum(Bytes& report);
    static bool validResponse(const Bytes& request, const Bytes& response);
};
