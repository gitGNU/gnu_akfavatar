-- this file is in the public domain -- AKFoerster

require "lua-akfavatar"
require "base64"

negative = avt.load_audio_string(base64.decode([[
LnNuZAAAABwAABPFAAAAAQAAH0AAAAABAAAAAH19fHj++/r5/fv6+Pn57/Dw8PPv7/Pz8e/t7/Tz
8/Pz9/bz9vb09ff4+fl++/rs3fNo9envdHHw/XVlUWjd+eDhb+bw6vhe3t17e1xfZlpdYmr+9+xy
Xu/j+nNlb2xn8HdfY2tvWU5VU1bcz2/yyMXLzcW+vr7EyMrgeH1URUhKPzw5Myws77syNrvGv8K/
rbaurby1zd67ZEJSRUlCOT89PUE1NjY8t8gz57nDw8uzr7attLq73srNSk1NR0c9NjM1MCkiOahK
I7u/WcPNrK+uorKwt36zxj56VkdMNC8zMzQqJycnvbchQa1vw7yxp6yhpK6swLevSU7ORjw1LTEv
LCsnJB4qqUYcvLLhtsOnpKidpaWpwaiyQOjzSEsvKy8tLCceHBgmpDUass/cr0Cin6WXoJ2iv6K3
PsBUU+coKjQrLSAbHhcbq74bTdc+sz60m5+XnZycuqOl77m+9sowKDgpKCYeIBwVDzelGCu0LqjV
15qglZialamgnLi4td65ViY0KSMlHB0YDw1MpBUmSyisQreZnZCXlZWrm5qsrbjHsE4sPionIxkY
FAwOuzwUUyhJrTCmopeQnI+ZoJWeqau+tqtGNTglJBoVFA8LHLkfIjQkyDffo6KPlZOQn5mapamv
tq7CODosJR8WEQ8MFrolHjodVzs9oKeRkZaPnp2ZoqqqtLO1PTsyIR0RDw0NMz0dNicveS+6pJqT
lpKXnZyfqayxt7BhOjYkGxEODBq9JiY6IlU/XK6plZeYl56en6i4tbq9wD9ELh0WEA4X2zQpSShi
d0uwrpuZnJqho6SuwsDYz8ZGVC8dFxAPHb8/ONo5vdLOq6ucmpyep6msvE1eWFhRMzkpGRQPFGK8
NcXi0a7Jq6afmJudpK2tt0U2Pzc8LScqGBAPEU22SbXBtqiyo5+dlJacoaqtsUAuMS4wJB4cEw4N
GdpaQ73Bq6innJuXkpOZnqapuTMtLSsqHRsUDQsNJj8w6HO2pqqdmpiRj5KXnZ+lxUQzLC8iHBcP
DQwXMicxRF2vs6idm5SRkpOYnJ2pylw2NCwfHBYSDg8jJB4vLFm+w6einJaXl5aam52wwV1EdzIn
HhgTEB8mHSolNnRExbOjnZ6bm52eoK26ysu32UArHxkZLyckLiY/PDhn7LGtq6SmpqSpuL7MwbbM
3T4xJx5DQjBIL05eO9xuw7fBs7S6trnG3F/MvMxPOi4tws9Dyj3VzEfDTMe22LzN2cpvST84QnZN
/Uc7PCrUvE+13ryszbLGyLPMu77ayvFJPTA1SUA+PzpBKyq7wdu766+xwbLKr67Ar8HKwG5LNTA9
PTg4OT5DPTkwLPu+8rrDsqm9sr6/r8G3utLG6FhENDo+OT03OD45LDXITXrL3qy+vrTBrb+8r8W6
yXlfOj5EOj07P1E9PT9DWUhd2+nQ4fDGtLrCtru6vd3N5+7ZU1FLS1hDOzs6Pkpo2tnJvb6/ysjB
zs3Pzcraz9prXlFRSz8/Pzw8MknLTeHf3rvvzsvrvcW+vM/CydHWb9/V7nhZT1FLS0dCQkVYW1p5
5M/Q19TTzczPz9fVztbd7m7+Y15dVFhWU1ZUWVpbaG996N7Y1Nba2tza2d7e5vpmU/vT6OPp9uRd
WltUZVtd8vXd2d/c5OXa4OLk8d/k5tfe5PN26/nu6vfo929yZmJiXF5iZG906+ft5/Hk3t/jdfXg
2dTa3Obp6XV5c2x0Zl9eXGFjY2Vic+zi3d7d3trV19fZ29nj5HBg2ud67GN0Wk9TSU9TWPxv5trc
1+Tl3+La2tjT1dTa4ev9b2JeWltdWl1eXl5cYm9+8Ojd19XU0dPS1drb3+Dh5+t9b2xiXFlYW1pZ
X2hseXr35uPe3dzZ29va29vg4+b2+3VqaWRgXl1dY2dkaGx58enf393a3Nvd39va2t/p7vx1b2lp
bGppZV1bW11kanrw7u9+fPv97uXf3eTt9XVraGlyefr+bmdfX19fa33u6+7t9/z1++/m39zd2+Lv
8Hp2ef3r7O3venFzc29vcnb67uro7v55enr97+vp6u/6e25rb3T97ero6/xua2ZgZG377Obf4eXo
7vd7+O3s5ujo7nx4aWdnb+Ta1dbd3+t4cGNeX2JkY2Vlburf3N7e3uHf72xtbW5uX19fXGBRX9jY
zs/X1ezwcWFmXnT5c/Hp39/j3ODr7PPq93VwZmpjX19bXmhtZ2Jy5dnU1dba5O3n7mpt8enucGhf
T0xOTlBZ3c3OzNDO0d7e6e/t7vPu493a53dkYGVgYWl28+Xp6+948/Po3dvQzs7P3N/q6+r57O3o
7G/9Z1JNSEZDRV3o2szKxsXJyc/R2NjS4+fsefNvcHNhXFZVTUNAQkZLYOfY0s3FwsHCxcTHzNHb
29vl/WVWVFdRTEdCPjo2NDY+aNzNwcS+vb+8wsDAx8bM0NHc43JYUU9PTkU+OzYxLi88dtnEvsC5
vLu4vbq6u7vHxcjY3WxeWk5QT0tCOjo1LiwpLD7w0b28vre5uLe6trWztLu8vcfY5uLsYlJKQjwx
LCYiIB80Vlm1yL203K6xtKiyp6qzrbq5v3fPa1dcQkc6LiwkHx0YHjU8yL/Osd29r7Ojq6Whq6eu
sLHFvsfR0WJpVjs0KyckHRwYGjU8yLPVsuLatb+ipKKdp6Gorq2/vsPazlxLSDUyKygsJykmHh4X
I9JRqbDCqUy0q6+cp5ycqaGvr7Llwthu7z1BOCwrIiAfGhkVEyc9zai4qbHIqrmgnqKZop+hrqrD
zshf1FpJWDo5MCcoIR8fGhkWFzHqsqWwqLC7qq6fnp+bpKSpsbDO2dZo80xLTzo3LigoIiQkICEc
Gxkh4cuqqLGkuq2lp5ugnp6rp7G2ttLI0fnXZPtcPD40LiwmKSgnKB4dGBk+Yq+jr5+vs6aunKGf
nKijrbKvycPT8NdVcl08Oy8uLCQmIyAfHBwaFhgoVrumqqaqtqmsnpyemaCip7Cvv8XIzcPXX1NB
OjArKygoJSIjHRwYFxYaScWlnqedrqynrJqenJqloq2vsdHFz9fRWV1OPDkvLCsnJyQiHxwaFhUX
K920oaeiqLKnrJ6cnZqjoqmurL7Bxs7D2OVpRD81Ly4oJyUiHxwaFhUTGzruqaWnoLCqqaWanZmb
oaKrq7C9vMjIy+bxUz04LiwrJCMgHh0aFxYUFy3tr6Ckn6aspaicm5uYn5+nra2/wMrYzm5dTjs2
LSkpJCMiHx0aFhUTGCt6rqCkn6aspaebm5uZoKGprq68vsbSzOFtWz86Mi0sJyUjHx0bFxMTFCjt
uJ6loKW1p66fm5yYn6Glray5v7/KxM5yWj03MiwrJyQiHhwaFhMTFCZLvaOpoaexpaqdm5uYn6Cl
q6q0ubzCwc/vYEA7NC4tJyUhHhwaFhMTFSt8sZ+ln6aupaqdm5uZn6Cnqqy3ur3Bw9L4Tzw3Lywq
JCIfHBsXFBIRHUDKpaSlpLesraecnZmcoaStq7G6u8G+xttmQjk0LS0qJSMeHBoUExIWLluwoqui
rrOprp2cmpifn6erqrK0ubq6xtZfPjoxLSwmIx8cGRMTERUuSK+krKOztKywnp2amZ+gqKurs7O3
urnG12c/PDEtKyYlIBwbFRQSFCpDt6Wto7C2rLKfnpuZn6Gpra22t7m7u8nhXD88Mi0sJiUhHRsW
FBITJT3Bp62mrbmts6KdnJmeoKesrLO1uLq5wc3lSUA5MC8pJiMeHRgUFBIeO/2qrKqnu6yvqJ2e
mp6hpa2ssbe3u7m8zOJMPzsxLyomIx4dGRUUERkuSq+qrKS0rq6snp+bnKCiq6uutba6urvEz11C
PDMwLCckHx0aFRMSFCQ9w6espKu1q7Cin52an6GorKy0trm7ucDJ4klCOTIvKigkHx0ZFBQSGTNU
rKaqorWurq6goJucoKKrq661t7y6vMXOXkM8MjIuKigiHxwYFRMSHDRprKqppratrqufn5qcn6Kp
qa2ytLa3ub/IfUo/NTUuKigjHxsZFRMUFSlTu6Oqoqqyqa6inp2an6Ooq6yztre7ur7N5Uk/OjIx
KyonIiAbGhYUFhYqWbqiqKCnrqatoJ2cmqCiqKust7i8vr3I12tFPTcvLiopJyMhHBoYFRcXH0fJ
qKOkoKuop6WdnZudoqSqrLG6vMDBx91uTT86MS4sKikkIR4aGRYWGRwy1bGipaClqqSmnpycm6Ci
qK2uubvAxcPWdlQ/OjItLSopJiIfHBoZFhgaIVLBp6CjnqilpKScnZudoqStrrS/vsjKz21VQzk2
Li0sKCglIR8cGxkXGRkm77yioKCfq6aopZ2dm56ipq2utry9xMXM7llAODQuLSwqKicmIh4dGxkZ
Gh07ya2fo56jqKOpn56dnKOlq6+0v8LEy83dY049OTQuLissKygoIyIfHRsaHB4vya6fn56fp6Sn
oJ2enaOnrbS3wMfM0tl4VUo+ODQvMC8uLisrKScmIR8eHB0eL8uvoJ+foKmnqKSfoJ+kqa25vMjS
2OjsYU9NQj06NDQxMDEuLSwrKickIB0eHyNLvamfop+mq6mro6Khoamstb2/1NfX3dl2XE9APjg1
NjQ3NjQzLi0sKigkIB8gIzLNrqWhoaSpqammo6Ojp62zu77GzNLa3H1eUEhEPjs7Ozo5NzUyLi4t
KyglIR8fIStWt6mjpKWprKysp6Wkpauwtru+xsrLztHc7mVPSUM/Pjs6OTc0MC8tKyomIh8eICM2
y7ClpKWnra2tq6alo6etsrm7vsTHxsXJz9t8WExEPz08Ojg2NTMwLS0rKCUgHyAiMtuypqWkp6yu
rqynpaOmq7G5vMHIycnIy9LcaU9GQD47OTk4Nzg4NjIyLywqJiIiISc03bCppaWoq66urKilo6Wq
sLe8wcfLzMvQ1uhjTkZBPjw6OTg4NzY0MzEvLSsoJiUkJixFvq+op6mqr6+sq6emp6qxuL7FytDV
1Nzb3+1sTkhCPTw6Ozs7PT0+Ozo5NDEuLCkoKCkwRMewq6irrrCzr62pp6mrsLm+xcnNzczNzdTn
aFFIQ0FAPz9CREZHRkRAPTo3NDIwLiwrKi42TMi2rqytrq+vrKuqqayus7q+yMnO1dff6W1dUkpG
QUBAPz8/QENEREI/PTs6Ojo6NjIvLCwtM0Tfuq+srK6ur6+rq6qqra+4v8XP0Njf3/Z4bV5YTkhF
QkJBQUJBREdIS0hGREJFRkVGREM9NzQyNTc9T/zJv7y2tbKxsa6vrq+ys7m7vcHEycvO19zm9G5b
Uk1KSUdGR0ZFRkdHRkVFRkZHRUNDQkM/PT07Ojs8PEFNbNHKwLy6trSxr6+ur7K0ubu9v8HGyM7Y
3/VvX1hSTUlHRUNBQEJDQkJCQ0VHSUtLTE1NTEpMUE9ZV05NRUVISFNefeDbzs/NycrCwL67ure4
uru9vr6/vr/BxcvO2ut3YFxWT01JR0ZEQkA/Pz9BREVHSkxPUVRWVlpcY3Jwe21fWVVXUlJQUl5k
/e7m2dnV1tfRzMbAvru6ubq7vL29vr6+wMPJ0uFvW1JMSkhGREI/Pj4+P0FCRUhKTlNXXWFy9v59
/u7h3/dsaWZhVldXUllYWFpYY2166uXc2NrT0dLOzsrHxsTFxcXFxMfIyczO197n+3NoX1tWU09P
UE9PUlVaWVlaW19gYWVqZ2BcVVVWW3D7+n5qef1lYmV87/vt9u7n6t/l4N3f2t7e3uTj4uDd2tva
2tjW1tba29vf3tze3d7d3ePs+H1wbG1oYmFgYFxZV1dYV1pdXF1eXl5bXmRpevrw7/Tt9Xr/9e3q
7/P5/fZ3ffv65uXm7e/l5uHf3NjX09bZ2dnY2dfX2NjZ2+Hp6u3v8PP2d21uaGJgZn38+/f47vx6
9vju7e/p7fD6eHhsbXNvcG969vx+d2xxfvv7/Onj5+ro4enn4/Lt6u3tfXFvbHV1cvT0+vp1ent8
8url5uDe4uTi4eLg5erq7/V1bm5uenZ3dmdtcG/9d3n4+ezt8vH06Obr6Orn7Pj5fu7i5ev09PD7
/ffx7fL5fXBrbn7+e/zz8fb4eX7x8vD5/vfu7Px+9O3s9/p+cW529nNqdG54fWhrb3P7dfv5dvh9
/nZpdnx0a2drb25rb3T+e/XtdvXw6+n+5+no3u3k393Z3tzh4tvi5Onx6Pr17Xz19/Dxb3VwbHBu
bW13/vXt/Pv0/vTy/P37+3x1a2pvdW5udXZ++3Jve/jy9/zv6+jn8/To5uHn8urq6/R29vPy7/j0
fXr5eHd4dnx6eX779H51+PLx9/3+cH17bX51cfv37/z88/fzfPvz+e7s5+Po6ers6/Ds6u7r7Ozv
9/v89fTz7Ozu8e7r7/Pw7ejt9vv793h0enZ+/P74/fjyfH17fvl+9/r+9378+v30/nr7ffn3fe/t
7uvu6ezv7PP09P/3/Hd2c3d0cXBsa2pnZmhsdXpwa2t2dXP6dn32fP5xdn15+H58/n37eXV7/PDw
8Ozr6evp6Ovp7ezo8ezt9O/78u737vTy7/bu9Pbv8e3s7ezv6ufs6e3u7PTzfnl+cnhybXZ89fX7
8e/v7/Lt7+/p6efo6OXn5+nn5+Xi5ujy/ft5eXd3/fP4fXl0e3dydXB6/3Ntbm9vdXtubXR99Pv8
9fv4+/31+vj4/vf9+Pd5fnp++Hv7+371+fPu7+fr7Orv6Onr6uvo7vLv9/Lu8PX6++/u8fT6+/j4
+fTw8vX5+H56+fr49fTt8Q==
]]))

return negative
