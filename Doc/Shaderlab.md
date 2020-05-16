#pragma <passName>
vertex <vert function name>
fragment <pixel function name>
hull <hull function name, this can be empty>
domain <domain shader name, this can be empty>
cull back/front/off
zwrite on/off always/never
ztest less/lequal/greater/gequal/equal/nequal/never
conservative on/off  always/never
blend on/off always/never
stencil_readmask 0-255
stencil_writemask 0-255
stencil_zfail keep/zero/replace
stencil_fail keep/zero/replace
stencil_pass keep/zero/replace
stencil_comp less/lequal/greater/gequal/equal/nequal/never
macro <MACRO>
#end