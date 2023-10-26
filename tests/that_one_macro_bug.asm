;P>This is print 1: WTF_Hello1
;P>This is print 2: WTF_Hello1
;P>This is print 1: WTF_Hello2
;P>This is print 2: WTF_Hello2
;P>This is print 1: WTF_Hello3
;P>This is print 2: WTF_Hello3
;P>This is print 1: WTF_Hello4
;P>This is print 2: WTF_Hello4
;P>This is print 1: Hello1
;P>This is print 2: Hello1
;P>This is print 1: Hello2
;P>This is print 2: Hello2
;P>This is print 1: Hello3
;P>This is print 2: Hello3
;P>This is print 1: Hello4
;P>This is print 2: Hello4

macro do_something_with_arg(arg)
	!def_<arg>_1 = <arg>
	!def_<arg>_2 = <arg>

	print "This is print 1: !def_<arg>_1"
	print "This is print 2: !def_<arg>_2"
endmacro

macro call_do_something_with_arg(prefix)
	%do_something_with_arg("<prefix>Hello1")
	%do_something_with_arg("<prefix>Hello2")
	%do_something_with_arg("<prefix>Hello3")
	%do_something_with_arg("<prefix>Hello4")
endmacro

%call_do_something_with_arg("WTF_")
%call_do_something_with_arg("")
