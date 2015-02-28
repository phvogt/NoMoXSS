print qq|
    <pre>
		<form method="get" name="|.$testform.qq|" action="|.$actionurl.qq|">
		
		  Testinput:       <input type="text" name="testinput" value="|.$testvalue.qq|input" size="20"><br/>
		  Testinputhidden: <input type="text" name="testinputhidden" value="|.$testvalue.qq|inputhidden" size="20"><br/>
		  Testpass:        <input type="password" name="testpass" value="|.$testvalue.qq|pass" size="20"><br/>
		  Testradio:       <input type="radio" name="testradio" value="|.$testvalue.qq|radio" size="20"><br/>
		  Testcheck:       <input type="checkbox" name="testcheckbox" value="|.$testvalue.qq|checkbox" size="20"><br/>
		  Testarea:        <textarea rows="2" cols="10" name="testarea">|.$testvalue.qq|textarea</textarea><br/>
		  Testfileupload:  <input type="file" name="testfileupload" value="|.$testvalue.qq|fileupload" size="20"><br/>
		  Testbutton:      <input type="button" onclick="evilFunction();" name="testbutton" value="|.$testvalue.qq|button" size="20"><br/>
		  TestSelect:      <select id="testselect1" name="testselect" size="1">
							  <option value="3" selected>aaa</option>
							  <option id="testselect2" value="1">|.$testvalue.qq|select</option>
						   </select>
		  TestLink:        <a id="testlink" href="http://deathstar.wg.vlbg/test.html?oje">testlink</a>
		  <input type="submit" name="testsubmit" value="Abschicken"> <input type="reset" name="testreset" value="Abbrechen">
		</form>
    </pre>
|;

1;