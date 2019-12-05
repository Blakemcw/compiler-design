#include "symbol.h"
using namespace std;

string attributes_to_string (symbol* sym) {
   attr_bitset attributes = sym->attributes;
   string ret = "";

   if (attributes.any()) {
      for (int a = 0; a < attr::BITSET_SIZE; ++a) {
         
         if (not (attributes[(attr)a])) continue;
         switch ((attr)a) {
            case attr::VOID        : ret += "void ";       break;
            case attr::INT         : ret += "int ";        break;
            case attr::NULLPTR_T   : ret += "nullptr ";    break;
            case attr::STRING      : ret += "string ";     break;
            case attr::STRUCT      : ret += "struct ";     break;
            case attr::FUNCTION    : ret += "function ";   break;
            case attr::VARIABLE    : ret += "variable ";   break;
            case attr::FIELD       : ret += "field ";      break;
            case attr::TYPEID      : ret += "typeid ";     break;
            case attr::PARAM       : ret += "param ";      break;
            case attr::LOCAL       : ret += "local ";      break;
            case attr::LVAL        : ret += "lval ";       break;
            case attr::CONST       : ret += "const ";      break;
            case attr::VREG        : ret += "vreg ";       break;
            case attr::VADDR       : ret += "vaddr ";      break;
            case attr::ARRAY       : ret += "array ";      break;
         
            default:                                       break;
         }
      }
   }
   return ret;
}