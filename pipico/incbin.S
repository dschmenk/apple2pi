.section .time_critical.firmware
.global firmware
.type firmware, %object
.balign 4
firmware:
.incbin "../../pidrive/PIROM#062000"
