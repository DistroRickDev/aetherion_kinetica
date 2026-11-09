package logger

import "base:runtime"
import "core:log"
import sdl "vendor:sdl3"

default_context: runtime.Context

init_logger :: proc() {
	context.logger = log.create_console_logger()
	default_context = context

	sdl.SetLogPriorities(.VERBOSE)
	sdl.SetLogOutputFunction(
		proc "c" (
			user_data: rawptr,
			category: sdl.LogCategory,
			priority: sdl.LogPriority,
			message: cstring,
		) {
			context = default_context
			log.debugf("SDL | {} | [{}]: {}", category, priority, message)
		},
		nil,
	)
}

debug ::  proc(fmt_str: string, args: ..any, location := #caller_location) {
	log.debugf(fmt_str, args, location)
}

error ::  proc(fmt_str: string, args: ..any, location := #caller_location) {
	log.errorf(fmt_str, args, location)
}

info ::  proc(fmt_str: string, args: ..any, location := #caller_location) {
	log.infof(fmt_str, args, location)
}

warn ::  proc(fmt_str: string, args: ..any, location := #caller_location) {
	log.warnf(fmt_str, args, location)
}
