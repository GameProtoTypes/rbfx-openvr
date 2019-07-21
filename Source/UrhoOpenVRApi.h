#define URHO_OPENVR_EXPORT_API __declspec(dllexport)


#ifdef URHO_OPENVR_STATIC
#  define URHO_OPENVR_API
#  define URHO_OPENVR_NO_EXPORT
#else
#  ifndef URHO_OPENVR_API
#    ifdef URHO_OPENVR_EXPORTS
/* We are building this library */
#      define URHO_OPENVR_API URHO_OPENVR_EXPORT_API
#    else
/* We are using this library */
#      define URHO_OPENVR_API __declspec(dllimport)
#    endif
#  endif

#  ifndef URHO_OPENVR_NO_EXPORT
#    define URHO_OPENVR_NO_EXPORT 
#  endif
#endif
