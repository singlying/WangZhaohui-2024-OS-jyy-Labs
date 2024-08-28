export TOKEN := ???

# ----- DO NOT MODIFY -----

export COURSE := OS2024
URL := 'https://jyywiki.cn/submit.sh'

submit:
	@cd $(dir $(abspath $(lastword $(MAKEFILE_LIST)))) && \
	  curl -sSLf '$(URL)' > /dev/null && \
	  curl -sSLf '$(URL)' | bash

