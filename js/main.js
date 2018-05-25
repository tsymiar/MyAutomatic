var $form_elem = null;
$(document).ready(function(){
    var options = {
        onFail: function(){
            alert("您还有"+ $form_elem.getInvalid().length +'处必填项未完成！' )
        },
        inputs: {
            // look at line 41
            'user[name]': {
                filters: 'required username',
                data: {
                //ajax: { url:'validate.php' }
                }
            },
            'file': {
                filters: 'extension',
                data: { extension: ['jpg'] }
            },
            'comments': {
                filters: 'min max',
                data: { min: 30, max: 200 }
            },
            'states': {
                filters: 'exclude',
                data: { exclude: ['default'] },
                errors : {
                    exclude: '.'
                }
            },
            'langs[]': {
                filters: 'min max',
                data: { min: 2, max: 3 },
                errors: {
                    min: 'Check at least <strong>2</strong> options.',
                    max: 'No more than <strong>3</strong> options allowed.'
                }
            }
        }
    };
    addEventListener("load", function() { setTimeout(hideURLbar, 0); }, false); 
    function hideURLbar(){ window.scrollTo(0,1); } 
    $form_elem = $('#main-form').idealforms(options).data('idealforms');
    $('#reset').click(function(){
        $form_elem.reset().fresh().focusFirst()
    });
    $form_elem.focusFirst();
});

var formArray = new Array(
    "username",
    "passwd",
    "email",
    "date",
    "icon",
    "website",
    "zip",
    "comments"
);

function aes_encrypt(psw, key){
    return CryptoJS.AES.encrypt(psw,key).toString();
}

function pop_hide(){
    setPopDivNoScroll("clazz_pop_div", "id_pop_div", false);
}

function click2Submit(){
    $("#dontjump").html("");
    if($form_elem == null || $form_elem.getInvalid().length >= 1){
        return;
    }
    var key = null;
    var icon = "";
    var param = "";
    var arrVal = "";
    var link = "trans/service.php?action=register";
    for(var i=0; i<formArray.length; i++){
        var arrKey = formArray[i];
        arrVal = $("#"+arrKey).val();
        if(arrKey == "username"){
            key = arrVal;
        }
        if(arrKey == "passwd"){
            if(key != null){
                arrVal = aes_encrypt(arrVal,key);
            }
        }
        if(arrKey == "icon"){
            icon = arrVal = $(".ideal-file-filename").val();
        }
        if(i == formArray.length-1){
            param += (arrKey + "=" + arrVal);
        }else{
            param += (arrKey+"="+arrVal+"&");
        }
    }
    nativeXMLHttp("POST", link, param, function(rsp_text){
            if(rsp_text){
                if(rsp_text.indexOf("ERROR") == -1){
                    var json = JSON.parse(rsp_text);
                    if(json.code == 200){
                        alert(json.message);
                    }
                }else{
                    setPopDivNoScroll("clazz_pop_div", "id_pop_div", true, rsp_text);
                }
            }
        });
    if(icon != null && icon != ""){
        UpladFile("icon", "trans/service.php?action=file_uptoload");
    }
}
