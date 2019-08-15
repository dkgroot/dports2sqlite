IKIWIKIFILE?=${PKGNAME}.iwiki

. if !target(ikiwiki)
_EXTRACT_DEPENDS=${EXTRACT_DEPENDS:C/^[^ :]+:([^ :@]+)(@[^ :]+)?(:[^ :]+)?/\1/:O:u:C,(^[^/]),${PORTSDIR}/\1,}
_PATCH_DEPENDS=${PATCH_DEPENDS:C/^[^ :]+:([^ :@]+)(@[^ :]+)?(:[^ :]+)?/\1/:O:u:C,(^[^/]),${PORTSDIR}/\1,}
_FETCH_DEPENDS=${FETCH_DEPENDS:C/^[^ :]+:([^ :@]+)(@[^ :]+)?(:[^ :]+)?/\1/:O:u:C,(^[^/]),${PORTSDIR}/\1,}
_LIB_DEPENDS=${LIB_DEPENDS:C/^[^ :]+:([^ :@]+)(@[^ :]+)?(:[^ :]+)?/\1/:O:u:C,(^[^/]),${PORTSDIR}/\1,}
_BUILD_DEPENDS=${BUILD_DEPENDS:C/^[^ :]+:([^ :@]+)(@[^ :]+)?(:[^ :]+)?/\1/:O:u:C,(^[^/]),${PORTSDIR}/\1,} ${_LIB_DEPENDS}
_RUN_DEPENDS=${RUN_DEPENDS:C/^[^ :]+:([^ :@]+)(@[^ :]+)?(:[^ :]+)?/\1/:O:u:C,(^[^/]),${PORTSDIR}/\1,} ${_LIB_DEPENDS}

. if exists(${DESCR})
_DESCR=${DESCR}
. else
_DESCR=/dev/null
. endif

.  if defined(BUILDING_IKIWIKI)
PAGE_OUT=${IKIWIKIDIR}/${IKIWIKIFILE}
.  else
PAGE_OUT=/dev/stdout
.  endif

HTMLIFY=${SED} -e 's/&/\&amp;/g' -e 's/>/\&gt;/g' -e 's/</\&lt;/g'

.  if empty(FLAVORS) || defined(_DESCRIBE_WITH_FLAVOR)
ikiwiki:
	@(${ECHO_CMD} "# ${PKGNAME:Q}"; 						\
	${ECHO_CMD} ""; 								\
	if [ -f ${DESCR} ]; then							\
		while read one two; do							\
		case "$$one" in								\
			WWW:)   break;							\
			;;								\
		*) echo -n "$$one $$two ";						\
			;;								\
		esac;									\
		done < ${DESCR};							\
	fi;										\
	${ECHO_CMD} "";									\
	${ECHO_CMD} "";									\
	${ECHO_CMD} "* Version: ${PORTVERSION:Q}"; 					\
	${ECHO_CMD} "* Revision: ${PORTREVISION:Q}"; 					\
	${ECHO_CMD} "* Origin: ${PKGDIR:C/[^\/]+\/\.\.\///:C/[^\/]+\/\.\.\///:C/^.*\/([^\/]+\/[^\/]+)$/\\1/}";	\
	${ECHO_CMD} "* Categories: "; 						\
	for cat in $(CATEGORIES); do 							\
		${ECHO_CMD} "  - [[$${cat}|CATEGORIES#$${cat}]]"; 			\
	done; 										\
	${ECHO_CMD} "* Maintainer: [[${MAINTAINER:Q}|MAINTAINERS#${MAINTAINER:Q}]]";	\
	if [ -f ${DESCR} ]; then 							\
		${ECHO_CMD} -n "* URL: ";						\
		${AWK} '$$1 ~ /^WWW:/ {print $$2}' ${DESCR} | ${HEAD} -1;		\
	fi;										\
	${ECHO_CMD} "* Comment: ${COMMENT:Q}"; 						\
	) > ${PAGE_OUT}
.  else # empty(FLAVORS)
ikiwiki: ${FLAVORS:S/^/ikiwiki-/}
.   for f in ${FLAVORS}
ikiwiki-${f}:
	@cd ${.CURDIR} && ${SETENV} FLAVOR=${f} ${MAKE} -B -D_DESCRIBE_WITH_FLAVOR ikiwiki
.   endfor
.  endif # empty(FLAVORS)
. endif

