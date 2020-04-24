#!/usr/bin/osascript
## This AppleScript wrapper allows dropping files on the application to be processed.

on run filelist
	process({})
end run

on open filelist
	process(filelist)
end open

on process(filelist)
	set cmd to quoted form of ((POSIX path of (path to me as text)) & "Contents/MacOS/focus-stack")
	
	if filelist is {} then
		set filelist to choose file with prompt "Select images to combine" of type {"public.image"} with multiple selections allowed
	end if
	
	set outpath to choose file name with prompt "Save result as:" default name "output.jpg"
	set outpath to quoted form of POSIX path of outpath
	
	set args_quoted to ""
	repeat with fname in filelist
		set args_quoted to args_quoted & " " & (quoted form of POSIX path of fname)
	end repeat
	
	set progress total steps to 100
	set progress completed steps to 0
	set progress description to "Processing images..."
	
	set cmd to cmd & " --output=" & outpath & " " & args_quoted & "&>/tmp/focus-stack.log & echo $!"
	do shell script cmd
	set pid to the result
	
	set status to ""
	repeat while "Saved to" is not in status
		try
			set status to last item of (read POSIX file "/tmp/focus-stack.log" using delimiter {linefeed, return})
			set progress description to status
			set progress completed steps to (((characters 2 thru 4) of status as text) as number)
			set progress total steps to (((characters 6 thru 8) of status as text) as number)
		end try
		try
			set logtext to (read POSIX file "/tmp/focus-stack.log")
			if (return & "[") is not in logtext and (number of logtext) > 100 then
				display alert "Error" message logtext as critical buttons {"Ok"}
				exit repeat
			end if
		end try
		delay 0.2
	end repeat
	
	do shell script "rm /tmp/focus-stack.log"
end process
